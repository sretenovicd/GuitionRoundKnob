#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_hardware_create(lv_obj_t *parent);
void page_hardware_update(uint8_t cpu_pct, uint8_t ram_pct, float net_down_mb, float net_up_mb);
void page_hardware_set_brightness(uint8_t brightness_pct);
