#pragma once

#include <Arduino.h>

void audio_buzzer_init();
void audio_buzzer_update(); // Call in loop()

// Triggers rhythmic notification chime / beep alert pulses for duration_ms (e.g. 5000ms)
void audio_buzzer_trigger_alert(uint32_t duration_ms = 5000);
