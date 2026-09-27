#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_shortcuts_create(lv_obj_t *parent);
void page_shortcuts_on_knob(int delta);
void page_shortcuts_update();
void page_shortcuts_reset();
void page_shortcuts_set_name(int index, const char *name);
int page_shortcuts_get_active();
