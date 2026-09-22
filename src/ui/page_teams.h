#pragma once

#include <Arduino.h>
#include <lvgl.h>

void page_teams_create(lv_obj_t *parent);
void page_teams_update(bool in_meeting, bool is_muted, bool hand_raised);
