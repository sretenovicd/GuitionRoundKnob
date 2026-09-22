#include <Arduino.h>
#include "config.h"
#include "hal/display_st77916.h"
#include "hal/touch_cst816.h"
#include "hal/knob_pulses.h"
#include "hal/led_ring.h"
#include "hal/usb_manager.h"
#include "ui/ui_manager.h"
#include "protocol/serial_protocol.h"

void setup() {
    Serial.begin(115200);

    // Initialize USB Composite (CDC Serial + HID Consumer)
    usb_manager_init();

    // Initialize Hardware Peripherals
    led_ring_init();
    knob_init();
    touch_init();
    display_init();

    // Initialize UI and Protocol
    ui_init();
    protocol_init();

    Serial.println("\n=====================================================");
    Serial.println(" Guition JC3636K718C Smart Knob Controller Ready");
    Serial.println(" Serial Debug Console Active. Type 'help' for commands.");
    Serial.println("=====================================================");
    Serial.println("{\"device\":\"Guition-JC3636K718C\",\"status\":\"ready\"}");
}

void loop() {
    knob_update();
    ui_update();
    protocol_update();
    led_ring_update();
    delay(5);
}
