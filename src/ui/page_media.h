#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_media_create(lv_obj_t *parent);
void page_media_update(const char *title, const char *artist, bool is_playing, uint32_t pos_ms, uint32_t duration_ms);
void page_media_set_accent_color(uint8_t r, uint8_t g, uint8_t b);
