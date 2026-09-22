#pragma once

#include <Arduino.h>

// ==========================================
// Guition JC3636K718C Pinout & Hardware Defs
// ==========================================

// --- Display QSPI (ST77916, 360x360 Round) ---
#define PIN_LCD_SCK   11
#define PIN_LCD_CS    12
#define PIN_LCD_D0    13
#define PIN_LCD_D1    14
#define PIN_LCD_D2    15
#define PIN_LCD_D3    16
#define PIN_LCD_RST   17
#define PIN_LCD_BLK   21   // PWM backlight

#define LCD_WIDTH     360
#define LCD_HEIGHT    360

// --- Touchscreen CST816 (I2C) ---
#define PIN_TOUCH_SDA  9
#define PIN_TOUCH_SCL 10
#define PIN_TOUCH_INT  7
#define PIN_TOUCH_RST  8

// --- Rotary Knob (Pulse-based, NOT quadrature) ---
// Left rotation sends clean LOW pulse to PIN_KNOB_LEFT (GPIO2)
// Right rotation sends clean LOW pulse to PIN_KNOB_RIGHT (GPIO1)
#define PIN_KNOB_LEFT   2
#define PIN_KNOB_RIGHT  1

// --- Addressable WS2812 LED Ring ---
#define PIN_LED_RING    0   // GPIO0 (also BOOT strapping pin)
#define LED_RING_COUNT 13

// --- Audio DAC PCM5100A & Mute ---
#define PIN_DAC_BCK    3
#define PIN_DAC_WS    45
#define PIN_DAC_DO    42
#define PIN_DAC_MUTE  46   // LOW = Mute, HIGH = Active

// --- Battery ADC ---
#define PIN_BAT_ADC    6
