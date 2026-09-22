#include "touch_cst816.h"
#include <Wire.h>

#define CST816_I2C_ADDR 0x15

void touch_init() {
    pinMode(PIN_TOUCH_RST, OUTPUT);
    pinMode(PIN_TOUCH_INT, INPUT);

    // Reset touch controller
    digitalWrite(PIN_TOUCH_RST, LOW);
    delay(20);
    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(50);

    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);
}

bool touch_read(int16_t *x, int16_t *y, bool *pressed, TouchGesture *gesture) {
    *pressed = false;
    if (gesture) *gesture = GESTURE_NONE;

    Wire.beginTransmission(CST816_I2C_ADDR);
    Wire.write(0x01); // Start from gesture/points registers
    if (Wire.endTransmission() != 0) {
        return false;
    }

    uint8_t data[6] = {0};
    uint8_t bytesRead = Wire.requestFrom((uint8_t)CST816_I2C_ADDR, (uint8_t)6);
    if (bytesRead < 6) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        data[i] = Wire.read();
    }

    uint8_t gesture_id = data[0];
    uint8_t points = data[1];

    if (gesture) {
        *gesture = (TouchGesture)gesture_id;
    }

    if (points > 0) {
        *pressed = true;
        *x = ((int16_t)(data[2] & 0x0F) << 8) | data[3];
        *y = ((int16_t)(data[4] & 0x0F) << 8) | data[5];

        // Bounds clamp for 360x360 screen
        if (*x < 0) *x = 0;
        if (*x >= LCD_WIDTH) *x = LCD_WIDTH - 1;
        if (*y < 0) *y = 0;
        if (*y >= LCD_HEIGHT) *y = LCD_HEIGHT - 1;
        return true;
    }

    return false;
}
