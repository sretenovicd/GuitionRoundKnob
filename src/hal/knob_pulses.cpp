#include "knob_pulses.h"
#include <esp_timer.h>

enum KnobResult : int8_t {
    KNOB_REJECT_PIN_HIGH = -3,
    KNOB_REJECT_OPPOSITE = -2,
    KNOB_REJECT_SAME     = 0,
    KNOB_ACCEPT_CW       = 1,
    KNOB_ACCEPT_CCW      = -1
};

struct KnobEvent {
    uint8_t pin;
    uint8_t p1_val;
    uint8_t p2_val;
    int8_t result;
    uint16_t dt_same_ms;
    uint16_t dt_opp_ms;
    int32_t total;
};

#define EVENT_BUF_SIZE 32
static KnobEvent event_buf[EVENT_BUF_SIZE];
static volatile int event_head = 0;
static volatile int event_tail = 0;

static volatile int knob_delta = 0;
static volatile int32_t knob_total = 0;
static KnobCallback user_callback = nullptr;

// Configurable debounce / lockout thresholds (in microseconds)
// 80ms same-pin lockout filters mechanical contact chatter on the active pin.
// 150ms opposite-pin lockout suppresses mechanical cross-talk and wiper dragging on the other pin.
static volatile int64_t same_lockout_us = 80000;   // 80ms
static volatile int64_t opp_lockout_us  = 150000;  // 150ms

static volatile int64_t last_left_time  = 0;
static volatile int64_t last_right_time = 0;

// Diagnostics counters
static volatile uint32_t count_cw_accepted     = 0;
static volatile uint32_t count_ccw_accepted    = 0;
static volatile uint32_t count_same_rejected   = 0;
static volatile uint32_t count_opp_rejected    = 0;
static volatile uint32_t count_glitch_rejected = 0;

static bool debug_enabled = true;

static void IRAM_ATTR push_event(uint8_t pin, uint8_t p1, uint8_t p2, int8_t res, uint16_t dt_same, uint16_t dt_opp, int32_t tot) {
    int next = (event_head + 1) % EVENT_BUF_SIZE;
    if (next != event_tail) {
        event_buf[event_head].pin = pin;
        event_buf[event_head].p1_val = p1;
        event_buf[event_head].p2_val = p2;
        event_buf[event_head].result = res;
        event_buf[event_head].dt_same_ms = dt_same;
        event_buf[event_head].dt_opp_ms = dt_opp;
        event_buf[event_head].total = tot;
        event_head = next;
    }
}

static void IRAM_ATTR isr_knob_left() {
    uint8_t p2 = digitalRead(PIN_KNOB_LEFT);
    uint8_t p1 = digitalRead(PIN_KNOB_RIGHT);
    int64_t now = esp_timer_get_time();

    uint16_t dt_same = (now - last_left_time > 65535000) ? 65535 : (uint16_t)((now - last_left_time) / 1000);
    uint16_t dt_opp  = (now - last_right_time > 65535000) ? 65535 : (uint16_t)((now - last_right_time) / 1000);

    // 1. Glitch filter: must currently read LOW
    if (p2 != LOW) {
        count_glitch_rejected = count_glitch_rejected + 1;
        push_event(PIN_KNOB_LEFT, p1, p2, KNOB_REJECT_PIN_HIGH, dt_same, dt_opp, knob_total);
        return;
    }

    // 2. Opposite pin lockout: if Right pin was triggered recently, Left is cross-talk
    if (now - last_right_time < opp_lockout_us) {
        count_opp_rejected = count_opp_rejected + 1;
        push_event(PIN_KNOB_LEFT, p1, p2, KNOB_REJECT_OPPOSITE, dt_same, dt_opp, knob_total);
        return;
    }

    // 3. Same pin bounce lockout: filter contact chatter
    if (now - last_left_time < same_lockout_us) {
        count_same_rejected = count_same_rejected + 1;
        push_event(PIN_KNOB_LEFT, p1, p2, KNOB_REJECT_SAME, dt_same, dt_opp, knob_total);
        return;
    }

    // Accepted CCW pulse
    last_left_time = now;
    knob_delta = knob_delta - 1;
    knob_total = knob_total - 1;
    count_ccw_accepted = count_ccw_accepted + 1;
    push_event(PIN_KNOB_LEFT, p1, p2, KNOB_ACCEPT_CCW, dt_same, dt_opp, knob_total);
}

static void IRAM_ATTR isr_knob_right() {
    uint8_t p1 = digitalRead(PIN_KNOB_RIGHT);
    uint8_t p2 = digitalRead(PIN_KNOB_LEFT);
    int64_t now = esp_timer_get_time();

    uint16_t dt_same = (now - last_right_time > 65535000) ? 65535 : (uint16_t)((now - last_right_time) / 1000);
    uint16_t dt_opp  = (now - last_left_time > 65535000) ? 65535 : (uint16_t)((now - last_left_time) / 1000);

    // 1. Glitch filter: must currently read LOW
    if (p1 != LOW) {
        count_glitch_rejected = count_glitch_rejected + 1;
        push_event(PIN_KNOB_RIGHT, p1, p2, KNOB_REJECT_PIN_HIGH, dt_same, dt_opp, knob_total);
        return;
    }

    // 2. Opposite pin lockout: if Left pin was triggered recently, Right is cross-talk
    if (now - last_left_time < opp_lockout_us) {
        count_opp_rejected = count_opp_rejected + 1;
        push_event(PIN_KNOB_RIGHT, p1, p2, KNOB_REJECT_OPPOSITE, dt_same, dt_opp, knob_total);
        return;
    }

    // 3. Same pin bounce lockout: filter contact chatter
    if (now - last_right_time < same_lockout_us) {
        count_same_rejected = count_same_rejected + 1;
        push_event(PIN_KNOB_RIGHT, p1, p2, KNOB_REJECT_SAME, dt_same, dt_opp, knob_total);
        return;
    }

    // Accepted CW pulse
    last_right_time = now;
    knob_delta = knob_delta + 1;
    knob_total = knob_total + 1;
    count_cw_accepted = count_cw_accepted + 1;
    push_event(PIN_KNOB_RIGHT, p1, p2, KNOB_ACCEPT_CW, dt_same, dt_opp, knob_total);
}

void knob_init() {
    pinMode(PIN_KNOB_LEFT, INPUT_PULLUP);
    pinMode(PIN_KNOB_RIGHT, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_KNOB_LEFT), isr_knob_left, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_KNOB_RIGHT), isr_knob_right, FALLING);
}

int knob_get_delta() {
    noInterrupts();
    int d = knob_delta;
    knob_delta = 0;
    interrupts();
    return d;
}

void knob_update() {
    int d = knob_get_delta();
    if (d != 0 && user_callback) {
        user_callback(d);
    }

    // Process and print debug ring buffer safely from task context
    while (event_tail != event_head) {
        KnobEvent ev = event_buf[event_tail];
        event_tail = (event_tail + 1) % EVENT_BUF_SIZE;

        if (debug_enabled) {
            if (ev.result == KNOB_ACCEPT_CW) {
                Serial.printf("[KNOB] CW  (+1) | Pin=RIGHT(%d) | P1=%d P2=%d | dt_same=%ums dt_opp=%ums | Total: %ld\n",
                              ev.pin, ev.p1_val, ev.p2_val, ev.dt_same_ms, ev.dt_opp_ms, (long)ev.total);
            } else if (ev.result == KNOB_ACCEPT_CCW) {
                Serial.printf("[KNOB] CCW (-1) | Pin=LEFT(%d)  | P1=%d P2=%d | dt_same=%ums dt_opp=%ums | Total: %ld\n",
                              ev.pin, ev.p1_val, ev.p2_val, ev.dt_same_ms, ev.dt_opp_ms, (long)ev.total);
            } else if (ev.result == KNOB_REJECT_SAME) {
                Serial.printf("[KNOB] REJECTED Pin=%d [SAME-PIN BOUNCE: %ums < %lldms] | P1=%d P2=%d\n",
                              ev.pin, ev.dt_same_ms, (long long)(same_lockout_us / 1000), ev.p1_val, ev.p2_val);
            } else if (ev.result == KNOB_REJECT_OPPOSITE) {
                Serial.printf("[KNOB] REJECTED Pin=%d [OPPOSITE LOCKOUT: %ums < %lldms] | P1=%d P2=%d\n",
                              ev.pin, ev.dt_opp_ms, (long long)(opp_lockout_us / 1000), ev.p1_val, ev.p2_val);
            } else if (ev.result == KNOB_REJECT_PIN_HIGH) {
                Serial.printf("[KNOB] REJECTED Pin=%d [GLITCH: pin already HIGH] | P1=%d P2=%d\n",
                              ev.pin, ev.p1_val, ev.p2_val);
            }
        }
    }
}

void knob_set_callback(KnobCallback cb) {
    user_callback = cb;
}

void knob_set_debug(bool enable) {
    debug_enabled = enable;
    Serial.printf("[KNOB] Verbose debug logging %s\n", enable ? "ENABLED" : "DISABLED");
}

bool knob_get_debug() {
    return debug_enabled;
}

void knob_set_same_lockout_ms(uint32_t ms) {
    same_lockout_us = (int64_t)ms * 1000;
    Serial.printf("[KNOB] Same-pin bounce lockout set to %u ms\n", ms);
}

uint32_t knob_get_same_lockout_ms() {
    return (uint32_t)(same_lockout_us / 1000);
}

void knob_set_opp_lockout_ms(uint32_t ms) {
    opp_lockout_us = (int64_t)ms * 1000;
    Serial.printf("[KNOB] Opposite-pin cross-talk lockout set to %u ms\n", ms);
}

uint32_t knob_get_opp_lockout_ms() {
    return (uint32_t)(opp_lockout_us / 1000);
}

void knob_print_status() {
    Serial.println("------------- Knob Hardware Status -------------");
    Serial.printf("Current Pins: LEFT=GPIO%d (val=%d), RIGHT=GPIO%d (val=%d)\n",
                  PIN_KNOB_LEFT, digitalRead(PIN_KNOB_LEFT),
                  PIN_KNOB_RIGHT, digitalRead(PIN_KNOB_RIGHT));
    Serial.printf("Settings:     Same-Pin Lockout = %u ms, Opposite-Pin Lockout = %u ms\n",
                  knob_get_same_lockout_ms(), knob_get_opp_lockout_ms());
    Serial.printf("Debug Stream: %s\n", debug_enabled ? "ON" : "OFF");
    Serial.printf("Position:     Total Accumulated = %ld\n", (long)knob_total);
    Serial.printf("Accepted:     CW = %u, CCW = %u\n", count_cw_accepted, count_ccw_accepted);
    Serial.printf("Rejected:     Same Bounce = %u, Opposite Cross-talk = %u, Glitch = %u\n",
                  count_same_rejected, count_opp_rejected, count_glitch_rejected);
    Serial.println("------------------------------------------------");
}

void knob_reset_stats() {
    noInterrupts();
    knob_delta = 0;
    knob_total = 0;
    count_cw_accepted = 0;
    count_ccw_accepted = 0;
    count_same_rejected = 0;
    count_opp_rejected = 0;
    count_glitch_rejected = 0;
    event_head = 0;
    event_tail = 0;
    interrupts();
    Serial.println("[KNOB] Statistics and position reset to 0.");
}
