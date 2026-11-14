#pragma once

typedef enum {
    QIPIQ_WIFI_OFF,
    QIPIQ_WIFI_CONNECTING,
    QIPIQ_WIFI_CONNECTED,
    QIPIQ_WIFI_SETUP,
    QIPIQ_WIFI_ERROR,
} qipiq_wifi_state_t;

void qipiq_wifi_start(void);
void qipiq_wifi_request_setup(void);
qipiq_wifi_state_t qipiq_wifi_state(void);
const char *qipiq_wifi_setup_name(void);
const char *qipiq_wifi_setup_code(void);
