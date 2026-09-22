#pragma once

#include <Arduino.h>
#include "../config.h"

typedef void (*KnobCallback)(int delta);

void knob_init();
int knob_get_delta();
void knob_update(); // Call in loop() to safely dispatch callbacks outside of ISR
void knob_set_callback(KnobCallback cb);

// Serial Debugging and Runtime Tuning
void knob_set_debug(bool enable);
bool knob_get_debug();
void knob_set_same_lockout_ms(uint32_t ms);
uint32_t knob_get_same_lockout_ms();
void knob_set_opp_lockout_ms(uint32_t ms);
uint32_t knob_get_opp_lockout_ms();
void knob_print_status();
void knob_reset_stats();
