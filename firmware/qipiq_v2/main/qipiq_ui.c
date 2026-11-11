#include "qipiq_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "qipiq_audio.h"
#include "qipiq_sensors.h"
#include "qipiq_wifi.h"

LV_FONT_DECLARE(qipiq_font_18);

typedef enum {
    PAGE_HOME,
    PAGE_MENU,
    PAGE_TIMER,
    PAGE_GAMES,
    PAGE_GUESS,
    PAGE_REACTION,
    PAGE_SETTINGS,
    PAGE_WIFI,
} page_t;

typedef enum {
    REACTION_IDLE,
    REACTION_WAIT,
    REACTION_GO,
} reaction_t;

typedef struct {
    const char *name;
    uint32_t color;
    int eye;
    int mouth_width;
    int mouth_height;
} expression_t;

static const expression_t expressions[] = {
    {"Қызығу", 0x8ee3f5, 68, 28, 12},
    {"Қуаныш", 0x96eb9b, 42, 66, 27},
    {"Таңғалу", 0xffd080, 84, 34, 40},
    {"Ойлану", 0xa6a8ff, 54, 32, 10},
    {"Күлкі", 0xffacda, 29, 80, 35},
    {"Тыныштық", 0xa4dbdc, 53, 40, 15},
};
enum { EXPRESSION_SURPRISE = 2, EXPRESSION_SLEEP = 6 };
static const expression_t sleep_face = {"Ұйқы", 0x7d95b5, 11, 25, 12};
static const int timer_presets[] = {10, 60, 180, 300, 600};
static const int brightness_levels[] = {25, 60, 90};
static const int volume_levels[] = {0, 55, 80};
static const char *TAG = "qipiq_ui";

static lv_display_t *panel;
static lv_obj_t *content;
static lv_obj_t *header;
static lv_obj_t *power_label;
static lv_obj_t *nav_label;
static lv_obj_t *eyes[2];
static lv_obj_t *mouth;
static lv_obj_t *status_label;
static lv_obj_t *timer_action;
static lv_obj_t *timer_buttons[2];
static lv_obj_t *settings_labels[3];
static lv_obj_t *sensor_label;
static lv_obj_t *wifi_qr_frame;
static lv_obj_t *wifi_qr;
static lv_obj_t *wifi_details;
static lv_obj_t *wifi_action;
static page_t page;
static bool has_audio;
static bool motion_enabled = true;
static bool dimmed;
static bool sleeping;
static bool timer_finished;
static bool timer_running;
static bool sensor_seen;
static uint32_t seen_motion;
static size_t mood;
static size_t timer_preset = 1;
static uint8_t brightness_index = 1;
static uint8_t volume_index = 2;
static int64_t timer_left_ms = 60000;
static int64_t timer_deadline_ms;
static int64_t reaction_target_ms;
static int64_t reaction_start_ms;
static reaction_t reaction;
static unsigned secret_number;
static unsigned guesses;
static bool guess_won;
static int last_timer_second = -1;
static int rendered_expression = -1;
static int64_t next_power_render_ms;

static void show_page(page_t next);

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void load_settings(void)
{
    if (nvs_flash_init() != ESP_OK) {
        ESP_LOGW(TAG, "Settings storage unavailable");
        return;
    }
    nvs_handle_t handle;
    if (nvs_open("qipiq", NVS_READONLY, &handle) != ESP_OK) {
        return;
    }
    uint8_t saved;
    if (nvs_get_u8(handle, "brightness", &saved) == ESP_OK && saved < 3) {
        brightness_index = saved;
    }
    if (nvs_get_u8(handle, "volume", &saved) == ESP_OK && saved < 3) {
        volume_index = saved;
    }
    if (nvs_get_u8(handle, "motion", &saved) == ESP_OK) {
        motion_enabled = saved != 0;
    }
    nvs_close(handle);
}

static void save_setting(const char *key, uint8_t value)
{
    nvs_handle_t handle;
    if (nvs_open("qipiq", NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGW(TAG, "Could not open settings storage");
        return;
    }
    if (nvs_set_u8(handle, key, value) != ESP_OK || nvs_commit(handle) != ESP_OK) {
        ESP_LOGW(TAG, "Could not save %s", key);
    }
    nvs_close(handle);
}

static lv_obj_t *label_at(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &qipiq_font_18, 0);
    lv_label_set_text(label, text);
    lv_obj_align(label, align, x, y);
    return label;
}

static lv_obj_t *make_button(const char *text, int width, int height, lv_align_t align,
                             int x, int y, lv_event_cb_t callback, void *user_data)
{
    lv_obj_t *button = lv_button_create(content);
    lv_obj_set_size(button, width, height);
    lv_obj_align(button, align, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x243d60), LV_PART_MAIN);
    lv_obj_set_style_radius(button, 13, LV_PART_MAIN);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);
    return label_at(button, text, LV_ALIGN_CENTER, 0, 0);
}

static lv_obj_t *shape(int width, int height, int x, int y)
{
    lv_obj_t *object = lv_obj_create(content);
    lv_obj_remove_style_all(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(object, width, height);
    lv_obj_align(object, LV_ALIGN_CENTER, x, y);
    lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
    return object;
}

static void draw_face(void)
{
    static int previous_audio = -1;
    static bool previous_blink;
    const qipiq_audio_state_t audio = qipiq_audio_get_state();
    const int expression = sleeping ? EXPRESSION_SLEEP : (int)mood;
    const bool blink = now_ms() % 3200 < 110;
    if (expression == rendered_expression && (int)audio == previous_audio && blink == previous_blink) {
        return;
    }
    rendered_expression = expression;
    previous_audio = audio;
    previous_blink = blink;
    const expression_t *face = sleeping ? &sleep_face : &expressions[mood];
    const lv_color_t color = lv_color_hex(face->color);
    for (size_t i = 0; i < 2; i++) {
        lv_obj_set_height(eyes[i], blink ? 8 : face->eye);
        lv_obj_set_style_bg_color(eyes[i], color, LV_PART_MAIN);
    }
    lv_obj_set_size(mouth, face->mouth_width, audio == QIPIQ_AUDIO_PLAYING ? 34 : face->mouth_height);
    lv_obj_set_style_bg_color(mouth, color, LV_PART_MAIN);
    const char *label = face->name;
    switch (audio) {
    case QIPIQ_AUDIO_RECORDING: label = "Тыңдап тұрмын…"; break;
    case QIPIQ_AUDIO_PLAYING: label = "Дыбыс ойнатылып жатыр"; break;
    case QIPIQ_AUDIO_ERROR: label = "Дыбыс қатесі"; break;
    default: break;
    }
    lv_label_set_text(status_label, label);
}

static void change_expression(lv_event_t *event)
{
    (void)event;
    sleeping = false;
    mood = (mood + 1) % (sizeof(expressions) / sizeof(expressions[0]));
    draw_face();
    ESP_LOGI(TAG, "Expression %u", (unsigned)mood);
}

static void record_voice(lv_event_t *event)
{
    (void)event;
    qipiq_audio_request_loopback();
}

static void play_tone(lv_event_t *event)
{
    (void)event;
    qipiq_audio_request_tone();
}

static void open_page(lv_event_t *event)
{
    show_page((page_t)(uintptr_t)lv_event_get_user_data(event));
}

static void show_home(void)
{
    rendered_expression = -1;
    lv_obj_t *touch = lv_obj_create(content);
    lv_obj_remove_style_all(touch);
    lv_obj_set_size(touch, 300, 190);
    lv_obj_align(touch, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_remove_flag(touch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(touch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(touch, change_expression, LV_EVENT_SHORT_CLICKED, NULL);
    if (has_audio) {
        lv_obj_add_event_cb(touch, play_tone, LV_EVENT_LONG_PRESSED, NULL);
    }
    eyes[0] = shape(52, 68, -56, -65);
    eyes[1] = shape(52, 68, 56, -65);
    mouth = shape(28, 12, 0, 12);
    status_label = label_at(content, "", LV_ALIGN_BOTTOM_MID, 0, -76);
    if (has_audio) {
        make_button("Дауыс жазу (2 с)", 244, 48, LV_ALIGN_BOTTOM_MID, 0, -14, record_voice, NULL);
    }
    draw_face();
}

static void refresh_timer(void)
{
    const int second = (int)((timer_left_ms + 999) / 1000);
    if (second != last_timer_second) {
        last_timer_second = second;
        lv_label_set_text_fmt(status_label, "%02d:%02d", second / 60, second % 60);
    }
    const char *action = timer_running ? "Кідірту" : timer_finished ? "Қайта бастау" : "Бастау";
    if (strcmp(lv_label_get_text(timer_action), action) != 0) {
        lv_label_set_text(timer_action, action);
    }
    if (timer_running) {
        lv_obj_add_state(timer_buttons[0], LV_STATE_DISABLED);
        lv_obj_add_state(timer_buttons[1], LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(timer_buttons[0], LV_STATE_DISABLED);
        lv_obj_remove_state(timer_buttons[1], LV_STATE_DISABLED);
    }
}

static void set_timer_duration(void)
{
    timer_left_ms = timer_presets[timer_preset] * 1000;
    timer_running = false;
    timer_finished = false;
}

static void choose_timer(lv_event_t *event)
{
    const size_t count = sizeof(timer_presets) / sizeof(timer_presets[0]);
    const bool increase = (uintptr_t)lv_event_get_user_data(event) != 0;
    timer_preset = (timer_preset + (increase ? 1 : count - 1)) % count;
    set_timer_duration();
    refresh_timer();
}

static void toggle_timer(lv_event_t *event)
{
    (void)event;
    if (timer_running) {
        const int64_t remaining = timer_deadline_ms - now_ms();
        timer_left_ms = remaining > 0 ? remaining : 0;
        timer_finished = remaining <= 0;
        timer_running = false;
    } else {
        if (timer_finished) {
            set_timer_duration();
        }
        timer_deadline_ms = now_ms() + timer_left_ms;
        timer_running = true;
    }
    refresh_timer();
}

static void reset_timer(lv_event_t *event)
{
    (void)event;
    set_timer_duration();
    refresh_timer();
}

static void show_timer(void)
{
    status_label = label_at(content, "", LV_ALIGN_TOP_MID, 0, 43);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    lv_obj_t *less = make_button("-", 70, 46, LV_ALIGN_TOP_MID, -92, 34, choose_timer, NULL);
    lv_obj_t *more = make_button("+", 70, 46, LV_ALIGN_TOP_MID, 92, 34, choose_timer, (void *)1);
    timer_buttons[0] = lv_obj_get_parent(less);
    timer_buttons[1] = lv_obj_get_parent(more);
    timer_action = make_button("Бастау", 260, 53, LV_ALIGN_TOP_MID, 0, 113, toggle_timer, NULL);
    make_button("Бастапқы күй", 260, 53, LV_ALIGN_TOP_MID, 0, 183, reset_timer, NULL);
    label_at(content, "Мәзірден шықсаңыз да, таймер жүреді", LV_ALIGN_BOTTOM_MID, 0, -16);
    last_timer_second = -1;
    refresh_timer();
}

static void reset_guess(lv_event_t *event)
{
    (void)event;
    secret_number = esp_random() % 9 + 1;
    guesses = 0;
    guess_won = false;
    lv_label_set_text(status_label, "1 мен 9 арасындағы санды тап");
}

static void guess_number(lv_event_t *event)
{
    if (guess_won) {
        return;
    }
    const unsigned guess = (unsigned)(uintptr_t)lv_event_get_user_data(event);
    guesses++;
    if (guess == secret_number) {
        guess_won = true;
        lv_label_set_text_fmt(status_label, "Дұрыс! Әрекет саны: %u", guesses);
        ESP_LOGI(TAG, "Guess won in %u attempts", guesses);
    } else {
        lv_label_set_text(status_label, guess < secret_number ? "Үлкенірек сан таңда" : "Кішірек сан таңда");
    }
}

static void show_guess(void)
{
    status_label = label_at(content, "", LV_ALIGN_TOP_MID, 0, 12);
    reset_guess(NULL);
    for (unsigned number = 1; number <= 9; number++) {
        char text[] = {(char)('0' + number), '\0'};
        const int column = (int)((number - 1) % 3) - 1;
        const int row = (number - 1) / 3;
        make_button(text, 80, 48, LV_ALIGN_TOP_MID, column * 94, 46 + row * 57,
                    guess_number, (void *)(uintptr_t)number);
    }
    make_button("Жаңа ойын", 240, 45, LV_ALIGN_BOTTOM_MID, 0, -10, reset_guess, NULL);
}

static void reaction_tap(lv_event_t *event)
{
    (void)event;
    const int64_t now = now_ms();
    if (reaction == REACTION_WAIT) {
        reaction = REACTION_IDLE;
        lv_label_set_text(status_label, "Тым ерте! Қайта баста");
    } else if (reaction == REACTION_GO) {
        const long elapsed = (long)(now - reaction_start_ms);
        reaction = REACTION_IDLE;
        lv_label_set_text_fmt(status_label, "Уақыт: %ld мс", elapsed);
        ESP_LOGI(TAG, "Reaction result: %ld ms", elapsed);
    } else {
        reaction = REACTION_WAIT;
        reaction_target_ms = now + 1500 + esp_random() % 2500;
        lv_label_set_text(status_label, "Күте тұр…");
    }
}

static void refresh_settings(void)
{
    lv_label_set_text_fmt(settings_labels[0], "Жарықтық %d%%", brightness_levels[brightness_index]);
    lv_label_set_text_fmt(settings_labels[1], "Дыбыс %d%%", volume_levels[volume_index]);
    lv_label_set_text(settings_labels[2], motion_enabled ? "Қозғалыс реакциясы қосулы" : "Қозғалыс реакциясы өшірулі");
}

static void change_setting(lv_event_t *event)
{
    switch ((uintptr_t)lv_event_get_user_data(event)) {
    case 0:
        brightness_index = (brightness_index + 1) % 3;
        bsp_display_brightness_set(brightness_levels[brightness_index]);
        save_setting("brightness", brightness_index);
        break;
    case 1:
        volume_index = (volume_index + 1) % 3;
        qipiq_audio_set_volume(volume_levels[volume_index]);
        save_setting("volume", volume_index);
        break;
    case 2:
        motion_enabled = !motion_enabled;
        save_setting("motion", motion_enabled);
        break;
    }
    refresh_settings();
}

static void show_settings(void)
{
    for (uintptr_t row = 0; row < 3; row++) {
        settings_labels[row] = make_button("", 298, 52, LV_ALIGN_TOP_MID, 0, 18 + row * 66,
                                           change_setting, (void *)row);
    }
    sensor_label = label_at(content, "", LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_style_text_align(sensor_label, LV_TEXT_ALIGN_CENTER, 0);
    refresh_settings();
}

static void start_wifi_setup(lv_event_t *event)
{
    (void)event;
    qipiq_wifi_request_setup();
}

static void refresh_wifi(void)
{
    static int last_wifi_state = -1;
    const qipiq_wifi_state_t state = qipiq_wifi_state();
    if ((int)state == last_wifi_state && lv_label_get_text(status_label)[0] != '\0') {
        return;
    }
    last_wifi_state = state;
    lv_obj_add_flag(wifi_qr_frame, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifi_details, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(wifi_action, LV_OBJ_FLAG_HIDDEN);
    switch (state) {
    case QIPIQ_WIFI_SETUP: {
        const char *name = qipiq_wifi_setup_name();
        const char *code = qipiq_wifi_setup_code();
        char payload[160];
        const int length = snprintf(payload, sizeof(payload),
            "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"softap\","
            "\"security\":1,\"password\":\"%s\"}", name, code, code);
        lv_label_set_text_fmt(wifi_details, "%s\nКод: %s", name, code);
        lv_obj_remove_flag(wifi_details, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifi_action, LV_OBJ_FLAG_HIDDEN);
        if (length > 0 && (size_t)length < sizeof(payload) &&
            lv_qrcode_update(wifi_qr, payload, length) == LV_RESULT_OK) {
            lv_label_set_text(status_label, "Қолданбада QR кодты оқытыңыз");
            lv_obj_remove_flag(wifi_qr_frame, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Wi-Fi setup QR rendered");
        } else {
            lv_label_set_text(status_label, "Желіні қолмен таңдаңыз");
            ESP_LOGW(TAG, "Wi-Fi setup QR generation failed");
        }
        break;
    }
    case QIPIQ_WIFI_CONNECTING: lv_label_set_text(status_label, "Желіге қосылып жатыр…"); break;
    case QIPIQ_WIFI_CONNECTED: lv_label_set_text(status_label, "Wi-Fi қосылды"); break;
    case QIPIQ_WIFI_ERROR: lv_label_set_text(status_label, "Wi-Fi іске қосылмады"); break;
    default: lv_label_set_text(status_label, "Wi-Fi бапталмаған"); break;
    }
    // Replace the previous QR and status pixels on the partial-update display.
    lv_obj_invalidate(content);
}

static void go_back(lv_event_t *event)
{
    (void)event;
    if (page == PAGE_HOME) {
        show_page(PAGE_MENU);
    } else if (page == PAGE_MENU) {
        show_page(PAGE_HOME);
    } else if (page == PAGE_GUESS || page == PAGE_REACTION) {
        show_page(PAGE_GAMES);
    } else {
        show_page(PAGE_MENU);
    }
}

static void show_page(page_t next)
{
    static const char *const titles[] = {
        "QIPIQ", "Мәзір", "Таймер", "Ойындар", "Сан тап", "Шапшаңдық", "Баптаулар", "Wi-Fi",
    };
    lv_obj_clean(content);
    page = next;
    next_power_render_ms = 0;
    reaction = REACTION_IDLE;
    lv_label_set_text(header, titles[next]);
    lv_label_set_text(nav_label, next == PAGE_HOME ? "Мәзір" : "Артқа");
    switch (page) {
    case PAGE_HOME:
        show_home();
        break;
    case PAGE_MENU:
        make_button("Таймер", 292, 47, LV_ALIGN_TOP_MID, 0, 18, open_page, (void *)PAGE_TIMER);
        make_button("Ойындар", 292, 47, LV_ALIGN_TOP_MID, 0, 80, open_page, (void *)PAGE_GAMES);
        make_button("Баптаулар", 292, 47, LV_ALIGN_TOP_MID, 0, 142, open_page, (void *)PAGE_SETTINGS);
        make_button("Wi-Fi", 292, 47, LV_ALIGN_TOP_MID, 0, 204, open_page, (void *)PAGE_WIFI);
        break;
    case PAGE_TIMER:
        show_timer();
        break;
    case PAGE_GAMES:
        make_button("Сан тап", 292, 57, LV_ALIGN_TOP_MID, 0, 46, open_page, (void *)PAGE_GUESS);
        make_button("Шапшаңдық", 292, 57, LV_ALIGN_TOP_MID, 0, 136, open_page, (void *)PAGE_REACTION);
        break;
    case PAGE_GUESS:
        show_guess();
        break;
    case PAGE_REACTION:
        status_label = label_at(content, "Жасыл белгі шыққанда түрт", LV_ALIGN_TOP_MID, 0, 30);
        make_button("Бас", 260, 125, LV_ALIGN_CENTER, 0, 18, reaction_tap, NULL);
        break;
    case PAGE_SETTINGS:
        show_settings();
        break;
    case PAGE_WIFI: {
        status_label = label_at(content, "", LV_ALIGN_TOP_MID, 0, 9);
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
        wifi_qr_frame = lv_obj_create(content);
        lv_obj_remove_style_all(wifi_qr_frame);
        lv_obj_remove_flag(wifi_qr_frame, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(wifi_qr_frame, 212, 212);
        lv_obj_set_style_bg_color(wifi_qr_frame, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(wifi_qr_frame, LV_OPA_COVER, 0);
        lv_obj_align(wifi_qr_frame, LV_ALIGN_TOP_MID, 0, 43);
        wifi_qr = lv_qrcode_create(wifi_qr_frame);
        lv_qrcode_set_size(wifi_qr, 180);
        lv_qrcode_set_dark_color(wifi_qr, lv_color_black());
        lv_qrcode_set_light_color(wifi_qr, lv_color_white());
        lv_obj_center(wifi_qr);
        wifi_details = label_at(content, "", LV_ALIGN_TOP_MID, 0, 263);
        lv_obj_set_style_text_align(wifi_details, LV_TEXT_ALIGN_CENTER, 0);
        wifi_action = lv_obj_get_parent(make_button("Wi-Fi баптау", 270, 51, LV_ALIGN_TOP_MID, 0, 143,
                                                    start_wifi_setup, NULL));
        refresh_wifi();
        break;
    }
    }
    lv_obj_invalidate(lv_display_get_screen_active(panel));
    ESP_LOGI(TAG, "Page %d", next);
}

static void update_sensors(int64_t now)
{
    qipiq_sensors_t state;
    if (!qipiq_sensors_read(&state)) {
        return;
    }
    if (now >= next_power_render_ms) {
        next_power_render_ms = now + 1000;
        if (state.power_valid && state.battery_percent >= 0) {
            lv_label_set_text_fmt(power_label, "%d%%%s", state.battery_percent, state.charging ? " +" : "");
        } else {
            lv_label_set_text(power_label, state.power_valid && state.usb_power ? "USB" : "--");
        }
        if (page == PAGE_SETTINGS) {
            const char *power = !state.power_valid ? "Қуат күйі белгісіз" :
                state.charging ? "Батарея зарядталып жатыр" :
                state.battery_present ? "Батарея қосулы" : "Батарея анықталмады";
            lv_label_set_text_fmt(sensor_label, "%s\n%s", power,
                                  state.imu_valid ? "Қозғалыс сенсоры дайын" : "Қозғалыс сенсоры қолжетімсіз");
        }
    }
    if (sensor_seen && state.motion_count != seen_motion && motion_enabled && page == PAGE_HOME && !dimmed) {
        mood = EXPRESSION_SURPRISE;
        ESP_LOGI(TAG, "Motion detected");
    }
    sensor_seen = true;
    seen_motion = state.motion_count;
}

static void tick(lv_timer_t *timer)
{
    (void)timer;
    const int64_t now = now_ms();
    const bool showing_qr = page == PAGE_WIFI && qipiq_wifi_state() == QIPIQ_WIFI_SETUP;
    const bool should_dim = !showing_qr && lv_display_get_inactive_time(panel) >= 30000;
    if (should_dim != dimmed) {
        dimmed = should_dim;
        sleeping = dimmed;
        bsp_display_brightness_set(dimmed ? 12 : brightness_levels[brightness_index]);
    }
    if (timer_running) {
        timer_left_ms = timer_deadline_ms > now ? timer_deadline_ms - now : 0;
        if (timer_left_ms == 0) {
            timer_running = false;
            timer_finished = true;
            lv_label_set_text(header, "Уақыт бітті");
            qipiq_audio_request_tone();
            ESP_LOGI(TAG, "Timer complete");
        }
        if (page == PAGE_TIMER) {
            refresh_timer();
        }
    }
    if (page == PAGE_REACTION && reaction == REACTION_WAIT && now >= reaction_target_ms) {
        reaction_start_ms = now;
        reaction = REACTION_GO;
        lv_label_set_text(status_label, "ҚАЗІР!");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x96eb9b), 0);
    } else if (page == PAGE_REACTION && reaction != REACTION_GO) {
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xf1f6ff), 0);
    }
    update_sensors(now);
    if (page == PAGE_HOME) {
        draw_face();
    } else if (page == PAGE_WIFI) {
        refresh_wifi();
    }
}

void qipiq_ui_start(lv_display_t *display, bool audio_ready)
{
    panel = display;
    has_audio = audio_ready;
    load_settings();
    bsp_display_brightness_set(brightness_levels[brightness_index]);
    qipiq_audio_set_volume(volume_levels[volume_index]);

    lv_obj_t *screen = lv_display_get_screen_active(display);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0b1420), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf1f6ff), 0);
    lv_obj_set_style_text_font(screen, &qipiq_font_18, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    header = label_at(screen, "QIPIQ", LV_ALIGN_TOP_LEFT, 20, 20);
    power_label = label_at(screen, "--", LV_ALIGN_TOP_RIGHT, -20, 20);

    content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x0b1420), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_size(content, lv_pct(100), lv_display_get_vertical_resolution(display) - 115);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 108);

    lv_obj_t *nav = lv_button_create(screen);
    lv_obj_set_size(nav, 316, 51);
    lv_obj_align(nav, LV_ALIGN_TOP_MID, 0, 47);
    lv_obj_add_flag(nav, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(nav, lv_color_hex(0x284867), 0);
    lv_obj_set_style_radius(nav, 13, 0);
    lv_obj_add_event_cb(nav, go_back, LV_EVENT_CLICKED, NULL);
    nav_label = label_at(nav, "Мәзір", LV_ALIGN_CENTER, 0, 0);

    show_page(PAGE_HOME);
    lv_timer_create(tick, 70, NULL);
}
