#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_clock_create(lv_obj_t *parent);
void page_clock_update_time(int hour, int minute, int second);
void page_clock_update_date(const char *date_str, const char *day_str);
