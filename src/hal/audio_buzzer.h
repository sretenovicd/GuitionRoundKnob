#pragma once

#include <Arduino.h>

void audio_buzzer_init();
void audio_buzzer_update(); // Call in loop()

// Sets chime volume (0 - 100%, default 15% for soft gentle chime)
void audio_buzzer_set_volume(uint8_t vol_pct);

// Triggers rhythmic notification chime / beep alert pulses for duration_ms (e.g. 5000ms)
void audio_buzzer_trigger_alert(uint32_t duration_ms = 5000);

