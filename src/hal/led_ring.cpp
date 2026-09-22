#include "led_ring.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel strip(LED_RING_COUNT, PIN_LED_RING, NEO_GRB + NEO_KHZ800);

static LedMode current_mode = LED_MODE_IDLE_BREATHING;
static uint8_t accent_r = 0, accent_g = 180, accent_b = 255; // Default Cyan
static bool meeting_active = false;
static bool meeting_muted = true;
static bool meeting_hand = false;

static unsigned long volume_show_until = 0;
static uint8_t current_volume = 50;

void led_ring_init() {
    strip.begin();
    strip.setBrightness(60); // 0-255 comfortable desk brightness
    strip.show();
}

void led_ring_set_mode(LedMode mode) {
    current_mode = mode;
}

void led_ring_set_accent_color(uint8_t r, uint8_t g, uint8_t b) {
    accent_r = r;
    accent_g = g;
    accent_b = b;
}

void led_ring_set_meeting_state(bool in_meeting, bool muted, bool hand_raised) {
    meeting_active = in_meeting;
    meeting_muted = muted;
    meeting_hand = hand_raised;
}

void led_ring_show_volume(uint8_t volume_pct) {
    current_volume = volume_pct;
    volume_show_until = millis() + 2000; // Stay for 2 seconds
}

void led_ring_update() {
    static unsigned long last_update = 0;
    unsigned long now = millis();
    if (now - last_update < 20) return; // ~50 fps update
    last_update = now;

    // Check volume gauge temporary override
    if (now < volume_show_until) {
        int active_leds = (int)((current_volume * LED_RING_COUNT) / 100);
        for (int i = 0; i < LED_RING_COUNT; i++) {
            if (i < active_leds) {
                strip.setPixelColor(i, strip.Color(0, 200, 255)); // Bright cyan
            } else {
                strip.setPixelColor(i, strip.Color(0, 0, 0));
            }
        }
        strip.show();
        return;
    }

    // Meeting Status (Highest Priority)
    if (meeting_active) {
        uint32_t col;
        if (meeting_hand) {
            col = strip.Color(255, 200, 0); // Yellow
        } else if (meeting_muted) {
            col = strip.Color(255, 0, 0);   // Solid Red
        } else {
            col = strip.Color(0, 255, 50);  // Green
        }
        for (int i = 0; i < LED_RING_COUNT; i++) {
            strip.setPixelColor(i, col);
        }
        strip.show();
        return;
    }

    // Music Accent Color
    if (current_mode == LED_MODE_MUSIC_ACCENT) {
        for (int i = 0; i < LED_RING_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(accent_r, accent_g, accent_b));
        }
        strip.show();
        return;
    }

    // Idle Breathing (Default)
    static float breath_angle = 0;
    breath_angle += 0.03f;
    if (breath_angle > 6.28318f) breath_angle -= 6.28318f;

    float factor = (sinf(breath_angle) + 1.0f) * 0.5f; // 0.0 to 1.0
    uint8_t br = (uint8_t)(10 + factor * 70); // Gentle breathing brightness

    for (int i = 0; i < LED_RING_COUNT; i++) {
        // Soft blue-purple gradient
        strip.setPixelColor(i, strip.Color((uint8_t)(br * 0.4f), 0, (uint8_t)(br * 0.9f)));
    }
    strip.show();
}
