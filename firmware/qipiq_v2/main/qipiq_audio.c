#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bsp/esp-bsp.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qipiq_audio.h"

static const char *TAG = "qipiq_audio";
static TaskHandle_t audio_task;

enum {
    SAMPLE_RATE = 16000,
    TONE_HZ = 440,
    TONE_SAMPLES = SAMPLE_RATE,
    FADE_SAMPLES = SAMPLE_RATE / 100,
    FRAME_SAMPLES = 160,
    RECORD_SAMPLES = SAMPLE_RATE * 2,
    SPEAKER_VOLUME = 80,
    PLAY_TONE = 1,
    RECORD_AND_PLAY = 2,
};

static esp_codec_dev_sample_info_t audio_format = {
    .bits_per_sample = 16,
    .channel = 1,
    .sample_rate = SAMPLE_RATE,
    .mclk_multiple = 256,
};

static int open_speaker(esp_codec_dev_handle_t speaker)
{
    const int result = esp_codec_dev_open(speaker, &audio_format);
    if (result != ESP_CODEC_DEV_OK) {
        return result;
    }
    return esp_codec_dev_set_out_vol(speaker, SPEAKER_VOLUME);
}

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
            frame[i] = (int16_t)(12000.0f * gain * sinf(phase_step * sample));
        }
        const int result = esp_codec_dev_write(speaker, frame, sizeof(frame));
        if (result != ESP_CODEC_DEV_OK) {
            return result;
        }
    }
    return ESP_CODEC_DEV_OK;
}

static int record_and_play(esp_codec_dev_handle_t speaker, esp_codec_dev_handle_t microphone)
{
    const size_t bytes = RECORD_SAMPLES * sizeof(int16_t);
    int16_t *samples = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (samples == NULL) {
        return ESP_ERR_NO_MEM;
    }

    int result = esp_codec_dev_open(microphone, &audio_format);
    if (result == ESP_CODEC_DEV_OK) {
        result = esp_codec_dev_set_in_gain(microphone, 24.0f);
        if (result == ESP_CODEC_DEV_OK) {
            ESP_LOGI(TAG, "Recording for two seconds");
            result = esp_codec_dev_read(microphone, samples, bytes);
        }
        esp_codec_dev_close(microphone);
    }

    if (result == ESP_CODEC_DEV_OK) {
        int peak = 0;
        for (size_t i = 0; i < RECORD_SAMPLES; i++) {
            const int magnitude = abs(samples[i]);
            if (magnitude > peak) {
                peak = magnitude;
            }
        }
        ESP_LOGI(TAG, "Recorded %u bytes; peak %d", (unsigned)bytes, peak);
        result = open_speaker(speaker);
        if (result == ESP_CODEC_DEV_OK) {
            result = esp_codec_dev_write(speaker, samples, bytes);
        }
        esp_codec_dev_close(speaker);
    }
    free(samples);
    return result;
}

static void speaker_task(void *arg)
{
    esp_codec_dev_handle_t speaker = arg;
    esp_codec_dev_handle_t microphone = bsp_audio_codec_microphone_init();
    while (true) {
        uint32_t action;
        xTaskNotifyWait(0, UINT32_MAX, &action, portMAX_DELAY);
        int result;
        if (action == RECORD_AND_PLAY) {
            if (microphone == NULL) {
                ESP_LOGW(TAG, "Microphone unavailable");
                continue;
            }
            result = record_and_play(speaker, microphone);
        } else {
            result = open_speaker(speaker);
            if (result == ESP_CODEC_DEV_OK) {
                result = play_tone(speaker);
            }
            esp_codec_dev_close(speaker);
        }
        if (result == ESP_CODEC_DEV_OK) {
            ESP_LOGI(TAG, "%s", action == RECORD_AND_PLAY ? "Microphone playback sent" : "Test tone sent");
        } else {
            ESP_LOGW(TAG, "Audio operation failed: %d", result);
        }
    }
}

bool qipiq_audio_start(void)
{
    const i2s_std_config_t i2s_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = BSP_I2S_MCLK,
            .bclk = BSP_I2S_SCLK,
            .ws = BSP_I2S_LCLK,
            .dout = BSP_I2S_DOUT,
            .din = BSP_I2S_DSIN,
        },
    };
    if (bsp_audio_init(&i2s_config) != ESP_OK) {
        ESP_LOGW(TAG, "I2S initialization failed");
        return false;
    }
    esp_codec_dev_handle_t speaker = bsp_audio_codec_speaker_init();
    if (speaker == NULL) {
        ESP_LOGW(TAG, "Speaker initialization failed");
        return false;
    }

    if (xTaskCreate(speaker_task, "qipiq_audio", 4096, speaker, 4, &audio_task) == pdPASS) {
        ESP_LOGI(TAG, "Audio task ready");
        return true;
    }
    esp_codec_dev_delete(speaker);
    ESP_LOGW(TAG, "Could not start audio task");
    return false;
}

void qipiq_audio_request_tone(void)
{
    if (audio_task != NULL) {
        xTaskNotify(audio_task, PLAY_TONE, eSetValueWithoutOverwrite);
    }
}

void qipiq_audio_request_loopback(void)
{
    if (audio_task != NULL) {
        xTaskNotify(audio_task, RECORD_AND_PLAY, eSetValueWithoutOverwrite);
    }
}
