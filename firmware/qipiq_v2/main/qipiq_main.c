#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "qipiq_audio.h"
#include "qipiq_sensors.h"
#include "qipiq_ui.h"

void app_main(void)
{
    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
        ESP_LOGE("qipiq", "Display initialization failed");
        return;
    }

    const bool audio_ready = qipiq_audio_start();
    qipiq_sensors_start();
    if (!bsp_display_lock(1000)) {
        ESP_LOGE("qipiq", "Could not lock LVGL");
        return;
    }
    qipiq_ui_start(display, audio_ready);
    bsp_display_unlock();
    ESP_LOGI("qipiq", "Offline menu ready");
}
