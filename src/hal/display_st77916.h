#pragma once

#include <Arduino.h>
#include "config.h"

void display_init();
void display_set_backlight(uint8_t brightness_pct);
uint8_t display_get_backlight();
void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p);
