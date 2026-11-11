#include "qipiq_wifi.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

static const char *TAG = "qipiq_wifi";
static TaskHandle_t wifi_task;
static atomic_int state = QIPIQ_WIFI_OFF;
static char setup_name[20];
static char setup_code[13];
static atomic_bool connected;
static atomic_bool provisioning_ended;
static atomic_bool provisioning_failed;

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        atomic_store(&connected, true);
        ESP_LOGI(TAG, "Wi-Fi connected");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        atomic_store(&connected, false);
    } else if (base == WIFI_PROV_EVENT && id == WIFI_PROV_END) {
        atomic_store(&provisioning_ended, true);
    } else if (base == WIFI_PROV_EVENT && id == WIFI_PROV_CRED_FAIL) {
        atomic_store(&provisioning_failed, true);
    }
}

static bool wifi_init(void)
{
    if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK ||
        esp_netif_create_default_wifi_sta() == NULL || esp_netif_create_default_wifi_ap() == NULL) {
        return false;
    }
    const wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    return esp_wifi_init(&config) == ESP_OK &&
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event, NULL) == ESP_OK &&
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL) == ESP_OK &&
        esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL) == ESP_OK;
}

static bool start_station(void)
{
    wifi_config_t stored = {0};
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_get_config(WIFI_IF_STA, &stored) != ESP_OK || stored.sta.ssid[0] == 0) {
        esp_wifi_stop();
        atomic_store(&state, QIPIQ_WIFI_OFF);
        return false;
    }
    if (esp_wifi_start() != ESP_OK) {
        atomic_store(&state, QIPIQ_WIFI_ERROR);
        return false;
    }
    atomic_store(&state, QIPIQ_WIFI_CONNECTING);
    return true;
}

static bool start_setup(void)
{
    atomic_store(&provisioning_ended, false);
    atomic_store(&provisioning_failed, false);
    if (setup_code[0] == '\0') {
        if (esp_wifi_start() != ESP_OK) {
            return false;
        }
        // The Wi-Fi radio enables the hardware entropy source.
        snprintf(setup_code, sizeof(setup_code), "%08" PRIX32 "%04" PRIX32,
                 esp_random(), esp_random() & UINT32_C(0xffff));
    }
    esp_wifi_stop();
    atomic_store(&connected, false);
    const wifi_prov_mgr_config_t manager = {.scheme = wifi_prov_scheme_softap};
    if (wifi_prov_mgr_init(manager) != ESP_OK) {
        return false;
    }
    const esp_err_t result = wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_1, setup_code, setup_name, setup_code);
    if (result != ESP_OK) {
        wifi_prov_mgr_deinit();
        ESP_LOGW(TAG, "Provisioning start failed: %s", esp_err_to_name(result));
        return false;
    }
    atomic_store(&state, QIPIQ_WIFI_SETUP);
    ESP_LOGI(TAG, "Wi-Fi setup started");
    return true;
}

static void run_wifi(void *arg)
{
    (void)arg;
    if (!wifi_init()) {
        atomic_store(&state, QIPIQ_WIFI_ERROR);
        ESP_LOGW(TAG, "Wi-Fi setup unavailable");
        for (;;) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }

    bool station = start_station();
    bool provisioning = false;
    TickType_t setup_started = 0;
    TickType_t last_connect = xTaskGetTickCount() - pdMS_TO_TICKS(8000);

    while (true) {
        const bool setup_requested = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500)) != 0;
        const TickType_t now = xTaskGetTickCount();
        if (setup_requested && !provisioning) {
            provisioning = start_setup();
            station = false;
            setup_started = now;
            if (!provisioning) {
                atomic_store(&state, QIPIQ_WIFI_ERROR);
            }
        }
        if (provisioning) {
            if (atomic_exchange(&provisioning_failed, false)) {
                wifi_prov_mgr_reset_sm_state_on_failure();
            }
            if (atomic_exchange(&provisioning_ended, false)) {
                wifi_prov_mgr_deinit();
                provisioning = false;
                station = atomic_load(&connected) || start_station();
            } else if (now - setup_started >= pdMS_TO_TICKS(120000)) {
                wifi_prov_mgr_stop_provisioning();
            }
        }
        if (station && !provisioning) {
            const bool online = atomic_load(&connected);
            atomic_store(&state, online ? QIPIQ_WIFI_CONNECTED : QIPIQ_WIFI_CONNECTING);
            if (!online && now - last_connect >= pdMS_TO_TICKS(8000)) {
                last_connect = now;
                esp_wifi_connect();
            }
        }
    }
}

void qipiq_wifi_start(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(setup_name, sizeof(setup_name), "QIPIQ-%02X%02X", mac[4], mac[5]);
    if (xTaskCreate(run_wifi, "qipiq_wifi", 6144, NULL, 3, &wifi_task) != pdPASS) {
        atomic_store(&state, QIPIQ_WIFI_ERROR);
    }
}

void qipiq_wifi_request_setup(void)
{
    if (wifi_task != NULL) {
        xTaskNotifyGive(wifi_task);
    }
}

qipiq_wifi_state_t qipiq_wifi_state(void)
{
    return atomic_load(&state);
}

const char *qipiq_wifi_setup_name(void)
{
    return setup_name;
}

const char *qipiq_wifi_setup_code(void)
{
    return setup_code;
}
