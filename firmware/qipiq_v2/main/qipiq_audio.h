#pragma once

#include <stdbool.h>

typedef enum {
    QIPIQ_AUDIO_IDLE,
    QIPIQ_AUDIO_RECORDING,
    QIPIQ_AUDIO_PLAYING,
    QIPIQ_AUDIO_ERROR,
} qipiq_audio_state_t;

bool qipiq_audio_start(void);
void qipiq_audio_request_tone(void);
void qipiq_audio_request_loopback(void);
void qipiq_audio_set_volume(int volume);
qipiq_audio_state_t qipiq_audio_get_state(void);
