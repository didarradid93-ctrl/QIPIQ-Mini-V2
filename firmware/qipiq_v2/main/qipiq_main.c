#include "bsp/esp-bsp.h"
#include "bsp/touch.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "qipiq_audio.h"
#include "qipiq_sensors.h"
#include "qipiq_ui.h"
#include "qipiq_wifi.h"

static void align_panel_area(lv_event_t *event)
{
    lv_area_t *area = lv_event_get_param(event);
    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 |= 1;
    area->y2 |= 1;
}

static lv_display_t *start_display(void)
{
    const lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    const bsp_display_config_t panel_config = {0};
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_touch_handle_t touch;
    if (bsp_display_new(&panel_config, &panel, &io) != ESP_OK ||
        bsp_touch_new(NULL, &touch) != ESP_OK ||
        lvgl_port_init(&port_config) != ESP_OK) {
        return NULL;
    }

    // QSPI transfers complete asynchronously. Use the SPI I/O completion path
    // so LVGL cannot reuse a draw buffer while the panel is still reading it.
    const lvgl_port_display_cfg_t config = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = BSP_LCD_H_RES * 40,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {.buff_dma = true, .swap_bytes = true},
    };
    lv_display_t *display = lvgl_port_add_disp(&config);
    if (display == NULL) {
        return NULL;
    }
    lv_display_add_event_cb(display, align_panel_area, LV_EVENT_INVALIDATE_AREA, NULL);
    const lvgl_port_touch_cfg_t touch_config = {.disp = display, .handle = touch};
    if (lvgl_port_add_touch(&touch_config) == NULL || bsp_display_brightness_init() != ESP_OK) {
        return NULL;
    }
    return display;
}

void app_main(void)
{
    lv_display_t *display = start_display();
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
    qipiq_wifi_start();
    ESP_LOGI("qipiq", "Offline menu ready");
}
