#pragma once

#include <Arduino.h>
#include <lvgl.h>

void ui_init();
void ui_update(); // Call regularly in loop()
void ui_next_page();
void ui_prev_page();
void ui_set_page(int page_index);
int ui_get_current_page();
