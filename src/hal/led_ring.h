#pragma once

#include <Arduino.h>
#include "config.h"

enum LedMode {
    LED_MODE_IDLE_BREATHING,
    LED_MODE_MUSIC_ACCENT,
    LED_MODE_MEETING,
    LED_MODE_VOLUME_GAUGE,
    LED_MODE_POMODORO
};

void led_ring_init();
void led_ring_update(); // Call in loop()

void led_ring_set_mode(LedMode mode);
void led_ring_set_accent_color(uint8_t r, uint8_t g, uint8_t b);
void led_ring_set_meeting_state(bool in_meeting, bool muted, bool hand_raised);
void led_ring_show_volume(uint8_t volume_pct); // 0 - 100

// Stopwatch 10s flash: flashes all LEDs with specified color for duration_ms
void led_ring_flash_stopwatch(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms = 350);

// Pomodoro display: sets percentage (0-100) and gradient color
void led_ring_set_pomodoro_state(bool active, uint8_t pct, uint8_t r, uint8_t g, uint8_t b);

// Alarm: pulses red / gold when timer completes
void led_ring_trigger_alarm(uint32_t duration_ms = 4000);

