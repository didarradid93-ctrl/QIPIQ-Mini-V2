#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qipiq_audio.h"

static const char *TAG = "qipiq_audio";
static TaskHandle_t audio_task;

enum {
    SAMPLE_RATE = 16000,
    TONE_HZ = 660,
    TONE_SAMPLES = SAMPLE_RATE / 4,
    FADE_SAMPLES = SAMPLE_RATE / 100,
    FRAME_SAMPLES = 160,
};

static int play_tone(esp_codec_dev_handle_t speaker)
{
    int16_t frame[FRAME_SAMPLES];
    const float phase_step = 6.28318530718f * TONE_HZ / SAMPLE_RATE;
    for (size_t offset = 0; offset < TONE_SAMPLES; offset += FRAME_SAMPLES) {
        for (size_t i = 0; i < FRAME_SAMPLES; i++) {
            const size_t sample = offset + i;
            const size_t tail = TONE_SAMPLES - 1 - sample;
            const size_t edge = sample < tail ? sample : tail;
            const float gain = edge < FADE_SAMPLES ? (float)edge / FADE_SAMPLES : 1.0f;
            frame[i] = (int16_t)(5000.0f * gain * sinf(phase_step * sample));
        }
        const int result = esp_codec_dev_write(speaker, frame, sizeof(frame));
        if (result != ESP_CODEC_DEV_OK) {
            return result;
        }
    }
    return ESP_CODEC_DEV_OK;
}

static void speaker_task(void *arg)
{
    esp_codec_dev_handle_t speaker = arg;
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const int result = play_tone(speaker);
        if (result == ESP_CODEC_DEV_OK) {
            ESP_LOGI(TAG, "Test tone sent");
        } else {
            ESP_LOGW(TAG, "Speaker write failed: %d", result);
        }
    }
}

bool qipiq_audio_start(void)
{
    esp_codec_dev_handle_t speaker = bsp_audio_codec_speaker_init();
    if (speaker == NULL) {
        ESP_LOGW(TAG, "Speaker initialization failed");
        return false;
    }

    esp_codec_dev_sample_info_t format = {
        .bits_per_sample = 16,
        .channel = 1,
        .sample_rate = SAMPLE_RATE,
        .mclk_multiple = 256,
    };
    int result = esp_codec_dev_open(speaker, &format);
    if (result == ESP_CODEC_DEV_OK) {
        result = esp_codec_dev_set_out_vol(speaker, 35);
        if (result == ESP_CODEC_DEV_OK &&
            xTaskCreate(speaker_task, "qipiq_audio", 4096, speaker, 4, &audio_task) == pdPASS) {
            ESP_LOGI(TAG, "Speaker ready");
            return true;
        }
        esp_codec_dev_close(speaker);
    }
    esp_codec_dev_delete(speaker);
    ESP_LOGW(TAG, "Speaker setup failed: %d", result);
    return false;
}

void qipiq_audio_request_tone(void)
{
    if (audio_task != NULL) {
        xTaskNotifyGive(audio_task);
    }
}
