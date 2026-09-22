#pragma once

#include <Arduino.h>
#include "config.h"

enum LedMode {
    LED_MODE_IDLE_BREATHING,
    LED_MODE_MUSIC_ACCENT,
    LED_MODE_MEETING,
    LED_MODE_VOLUME_GAUGE
};

void led_ring_init();
void led_ring_update(); // Call in loop()

void led_ring_set_mode(LedMode mode);
void led_ring_set_accent_color(uint8_t r, uint8_t g, uint8_t b);
void led_ring_set_meeting_state(bool in_meeting, bool muted, bool hand_raised);
void led_ring_show_volume(uint8_t volume_pct); // 0 - 100
