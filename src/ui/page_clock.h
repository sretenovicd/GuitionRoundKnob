#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_clock_create(lv_obj_t *parent);
void page_clock_update_time(int hour, int minute, int second);
void page_clock_update_date(const char *date_str, const char *day_str);

bool page_clock_is_in_submode();
bool page_clock_is_in_pomodoro();
void page_clock_pomodoro_adjust(int delta);
void page_clock_return_to_normal();

