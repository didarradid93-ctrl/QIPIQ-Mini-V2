#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool power_valid;
    bool usb_power;
    bool battery_present;
    bool charging;
    int battery_percent;
    bool imu_valid;
    uint32_t motion_count;
} qipiq_sensors_t;

void qipiq_sensors_start(void);
bool qipiq_sensors_read(qipiq_sensors_t *snapshot);
