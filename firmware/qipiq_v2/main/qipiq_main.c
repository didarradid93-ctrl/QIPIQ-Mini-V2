#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp/esp-bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "qipiq_audio.h"

static const char *TAG = "qipiq";
static lv_obj_t *eyes[2];
static lv_obj_t *mouth;
static lv_obj_t *emotion_label;
static size_t emotion_index;
static bool blinking;

typedef struct {
    const char *name;
    uint32_t color;
    int eye_height;
    int mouth_width;
    int mouth_height;
} emotion_t;

static const emotion_t emotions[] = {
    {"Curious",   0x8ee3f5, 68, 28, 12},
    {"Happy",     0x96eb9b, 42, 66, 27},
    {"Surprised", 0xffd080, 84, 34, 40},
};

static void draw_expression(void)
{
    const emotion_t *emotion = &emotions[emotion_index];
    const lv_color_t color = lv_color_hex(emotion->color);
    for (size_t i = 0; i < 2; i++) {
        lv_obj_set_height(eyes[i], blinking ? 8 : emotion->eye_height);
        lv_obj_set_style_bg_color(eyes[i], color, LV_PART_MAIN);
    }
    lv_obj_set_size(mouth, emotion->mouth_width, emotion->mouth_height);
    lv_obj_set_style_bg_color(mouth, color, LV_PART_MAIN);
    lv_label_set_text(emotion_label, emotion->name);
}

static void on_tap(lv_event_t *event)
{
    (void)event;
    emotion_index = (emotion_index + 1) % (sizeof(emotions) / sizeof(emotions[0]));
    draw_expression();
    ESP_LOGI(TAG, "Touch: %s", emotions[emotion_index].name);
}

static void on_long_press(lv_event_t *event)
{
    (void)event;
    qipiq_audio_request_tone();
}

static void finish_blink(lv_timer_t *timer)
{
    (void)timer;
    blinking = false;
    draw_expression();
}

static void start_blink(lv_timer_t *timer)
{
    (void)timer;
    lv_timer_t *reopen = lv_timer_create(finish_blink, 110, NULL);
    if (reopen != NULL) {
        lv_timer_set_repeat_count(reopen, 1);
        blinking = true;
        draw_expression();
    }
}

static lv_obj_t *make_shape(lv_obj_t *parent, int width, int height, int x, int y)
{
    lv_obj_t *shape = lv_obj_create(parent);
    lv_obj_remove_style_all(shape);
    lv_obj_remove_flag(shape, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(shape, width, height);
    lv_obj_align(shape, LV_ALIGN_CENTER, x, y);
    lv_obj_set_style_radius(shape, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(shape, LV_OPA_COVER, LV_PART_MAIN);
    return shape;
}

static void create_face(lv_display_t *display, bool audio_ready)
{
    lv_obj_t *screen = lv_display_get_screen_active(display);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0b1420), LV_PART_MAIN);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf1f6ff), LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_tap, LV_EVENT_SHORT_CLICKED, NULL);
    if (audio_ready) {
        lv_obj_add_event_cb(screen, on_long_press, LV_EVENT_LONG_PRESSED, NULL);
    }

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "QIPIQ");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 34);

    eyes[0] = make_shape(screen, 52, 68, -54, -22);
    eyes[1] = make_shape(screen, 52, 68, 54, -22);
    mouth = make_shape(screen, 28, 12, 0, 59);
    emotion_label = lv_label_create(screen);
    lv_obj_align(emotion_label, LV_ALIGN_BOTTOM_MID, 0, -74);

    lv_obj_t *hint = lv_label_create(screen);
    lv_label_set_text(hint, audio_ready ? "Tap: expression  |  Hold: sound" : "Tap to change expression");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -38);

    draw_expression();
    lv_timer_create(start_blink, 3200, NULL);
}

void app_main(void)
{
    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    ESP_ERROR_CHECK(bsp_display_brightness_set(60));
    const bool audio_ready = qipiq_audio_start();

    if (!bsp_display_lock(1000)) {
        ESP_LOGE(TAG, "Could not lock LVGL");
        return;
    }
    create_face(display, audio_ready);
    bsp_display_unlock();
    ESP_LOGI(TAG, "QIPIQ face ready");
}
