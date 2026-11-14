#include "qipiq_sensors.h"

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "qmi8658.h"

static const char *TAG = "qipiq_sensors";
static QueueHandle_t snapshots;

enum {
    AXP2101_ADDRESS = 0x34,
    AXP2101_STATUS = 0x00,
    AXP2101_PERCENT = 0xa4,
    POLL_MS = 80,
    POWER_POLL_CYCLES = 25,
};

static i2c_master_dev_handle_t open_power_monitor(i2c_master_bus_handle_t bus)
{
    if (i2c_master_probe(bus, AXP2101_ADDRESS, 50) != ESP_OK) {
        return NULL;
    }
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AXP2101_ADDRESS,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t device = NULL;
    if (i2c_master_bus_add_device(bus, &config, &device) != ESP_OK) {
        return NULL;
    }
    return device;
}

static void update_power(i2c_master_dev_handle_t device, qipiq_sensors_t *state)
{
    const uint8_t status_reg = AXP2101_STATUS;
    uint8_t status[2];
    state->power_valid = device != NULL &&
        i2c_master_transmit_receive(device, &status_reg, 1, status, sizeof(status), 50) == ESP_OK;
    state->battery_percent = -1;
    if (!state->power_valid) {
        return;
    }

    // Read-only status fields, as documented by XPowersLib for AXP2101.
    state->battery_present = (status[0] & (1u << 3)) != 0;
    state->usb_power = (status[0] & (1u << 5)) != 0 && (status[1] & (1u << 3)) == 0;
    state->charging = state->battery_present && (status[1] >> 5) == 1;
    if (state->battery_present) {
        const uint8_t percentage_reg = AXP2101_PERCENT;
        uint8_t percentage;
        if (i2c_master_transmit_receive(device, &percentage_reg, 1, &percentage, 1, 50) == ESP_OK &&
            percentage <= 100) {
            state->battery_percent = percentage;
        }
    }
}

static bool start_imu(i2c_master_bus_handle_t bus, qmi8658_dev_t *imu)
{
    const uint8_t addresses[] = {QMI8658_ADDRESS_HIGH, QMI8658_ADDRESS_LOW};
    for (size_t i = 0; i < sizeof(addresses); i++) {
        if (i2c_master_probe(bus, addresses[i], 50) != ESP_OK) {
            continue;
        }
        if (qmi8658_init(imu, bus, addresses[i]) != ESP_OK ||
            qmi8658_write_register(imu, 0x60, 0xb0) != ESP_OK) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        qmi8658_set_accel_unit_mps2(imu, true);
        return qmi8658_write_register(imu, QMI8658_CTRL1, 0x60) == ESP_OK &&
            qmi8658_set_accel_range(imu, QMI8658_ACCEL_RANGE_4G) == ESP_OK &&
            qmi8658_set_accel_odr(imu, QMI8658_ACCEL_ODR_250HZ) == ESP_OK &&
            qmi8658_enable_sensors(imu, QMI8658_ENABLE_ACCEL) == ESP_OK;
    }
    return false;
}

static void sensor_task(void *arg)
{
    i2c_master_bus_handle_t bus = arg;
    i2c_master_dev_handle_t power = open_power_monitor(bus);
    qmi8658_dev_t imu = {0};
    const bool imu_started = start_imu(bus, &imu);
    qipiq_sensors_t state = {.battery_percent = -1};
    qmi8658_data_t previous = {0};
    bool have_previous = false;
    TickType_t last_motion = xTaskGetTickCount();
    ESP_LOGI(TAG, "AXP2101: %s, QMI8658: %s", power ? "ready" : "unavailable",
             imu_started ? "ready" : "unavailable");

    unsigned cycle = 0;
    while (true) {
        if (cycle == 0) {
            update_power(power, &state);
        }
        cycle = (cycle + 1) % POWER_POLL_CYCLES;
        if (imu_started) {
            bool ready = false;
            state.imu_valid = qmi8658_is_data_ready(&imu, &ready) == ESP_OK;
            if (state.imu_valid && ready) {
                qmi8658_data_t current = {0};
                state.imu_valid = qmi8658_read_sensor_data(&imu, &current) == ESP_OK;
                if (state.imu_valid) {
                    const float dx = current.accelX - previous.accelX;
                    const float dy = current.accelY - previous.accelY;
                    const float dz = current.accelZ - previous.accelZ;
                    const TickType_t now = xTaskGetTickCount();
                    if (have_previous && dx * dx + dy * dy + dz * dz > 36.0f &&
                        now - last_motion >= pdMS_TO_TICKS(1300)) {
                        state.motion_count++;
                        last_motion = now;
                    }
                    previous = current;
                    have_previous = true;
                }
            }
        }
        xQueueOverwrite(snapshots, &state);
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

void qipiq_sensors_start(void)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (bus == NULL) {
        return;
    }
    snapshots = xQueueCreate(1, sizeof(qipiq_sensors_t));
    if (snapshots != NULL && xTaskCreate(sensor_task, "qipiq_sensors", 4096, bus, 3, NULL) != pdPASS) {
        vQueueDelete(snapshots);
        snapshots = NULL;
    }
}

bool qipiq_sensors_read(qipiq_sensors_t *snapshot)
{
    return snapshots != NULL && xQueuePeek(snapshots, snapshot, 0) == pdPASS;
}
