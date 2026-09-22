#pragma once

#include <Arduino.h>
#include "config.h"

enum TouchGesture {
    GESTURE_NONE = 0,
    GESTURE_SLIDE_DOWN = 0x01,
    GESTURE_SLIDE_UP = 0x02,
    GESTURE_SLIDE_LEFT = 0x03,
    GESTURE_SLIDE_RIGHT = 0x04,
    GESTURE_CLICK = 0x05,
    GESTURE_DOUBLE_CLICK = 0x0B,
    GESTURE_LONG_PRESS = 0x0C
};

void touch_init();
bool touch_read(int16_t *x, int16_t *y, bool *pressed, TouchGesture *gesture = nullptr);
