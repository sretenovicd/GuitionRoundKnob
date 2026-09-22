#include "serial_protocol.h"
#include <ArduinoJson.h>
#include "../ui/page_clock.h"
#include "../ui/page_media.h"
#include "../ui/page_hardware.h"
#include "../ui/page_teams.h"
#include "../ui/ui_manager.h"
#include "../hal/led_ring.h"
#include "../hal/knob_pulses.h"
#include "../hal/usb_manager.h"
#include "../hal/display_st77916.h"

static String rx_line = "";

void protocol_init() {
    rx_line.reserve(512);
}

static void handle_command(const String &cmd) {
    String c = cmd;
    c.trim();
    if (c.equalsIgnoreCase("help") || c == "?") {
        Serial.println("\n========== Serial Debug & Monitor Commands ==========");
        Serial.println(" help                     : Show this help menu");
        Serial.println(" status                   : Show system & knob hardware status");
        Serial.println(" knob debug [on|off|1|0]  : Enable or disable verbose knob logs");
        Serial.println(" knob lockout <ms>        : Set same-pin bounce lockout (default 80ms)");
        Serial.println(" knob opplockout <ms>     : Set opposite-pin lockout (default 150ms)");
        Serial.println(" knob stats               : Show knob pulse & rejection counters");
        Serial.println(" knob reset               : Reset knob stats & position counters");
        Serial.println(" page <0-3>               : Switch UI page (0:Clock 1:Media 2:HW 3:Teams)");
        Serial.println(" backlight <0-100>        : Set display backlight brightness percentage");
        Serial.println(" vol up / vol down        : Simulate volume HID keystrokes");
        Serial.println(" restart                  : Reboot ESP32");
        Serial.println("=====================================================\n");
    } else if (c.equalsIgnoreCase("status")) {
        Serial.println("\n================ Device System Status ================");
        Serial.printf(" Free Heap:  %u KB (Total: %u KB, Min Free: %u KB)\n",
                      ESP.getFreeHeap() / 1024, ESP.getHeapSize() / 1024, ESP.getMinFreeHeap() / 1024);
        Serial.printf(" Free PSRAM: %u KB (Total: %u KB)\n",
                      ESP.getFreePsram() / 1024, ESP.getPsramSize() / 1024);
        Serial.printf(" Uptime:     %lu seconds\n", (unsigned long)(millis() / 1000));
        Serial.printf(" UI Page:    %d / 3\n", ui_get_current_page());
        Serial.printf(" Backlight:  %d%%\n", display_get_backlight());
        knob_print_status();
        Serial.println("=====================================================\n");
    } else if (c.startsWith("knob debug")) {
        if (c.indexOf("off") > 0 || c.indexOf("0") > 0) {
            knob_set_debug(false);
        } else {
            knob_set_debug(true);
        }
    } else if (c.startsWith("knob lockout")) {
        int ms = c.substring(12).toInt();
        if (ms > 0 && ms <= 1000) {
            knob_set_same_lockout_ms((uint32_t)ms);
        } else {
            Serial.println("[CMD] Usage: knob lockout <10-1000 ms>");
        }
    } else if (c.startsWith("knob opplockout")) {
        int ms = c.substring(15).toInt();
        if (ms > 0 && ms <= 1000) {
            knob_set_opp_lockout_ms((uint32_t)ms);
        } else {
            Serial.println("[CMD] Usage: knob opplockout <10-1000 ms>");
        }
    } else if (c.equalsIgnoreCase("knob stats") || c.equalsIgnoreCase("knob status")) {
        knob_print_status();
    } else if (c.equalsIgnoreCase("knob reset")) {
        knob_reset_stats();
    } else if (c.startsWith("page ")) {
        int p = c.substring(5).toInt();
        if (p >= 0 && p < 4) {
            ui_set_page(p);
            Serial.printf("[CMD] Switched to UI page %d\n", p);
        } else {
            Serial.println("[CMD] Valid pages: 0=Clock, 1=Media, 2=Hardware, 3=Teams");
        }
    } else if (c.startsWith("backlight ") || c.startsWith("bri ")) {
        int b = c.substring(c.indexOf(' ') + 1).toInt();
        if (b >= 0 && b <= 100) {
            display_set_backlight((uint8_t)b);
            page_hardware_set_brightness((uint8_t)b);
            led_ring_show_volume((uint8_t)b);
            Serial.printf("[CMD] Display backlight set to %d%%\n", b);
        } else {
            Serial.println("[CMD] Usage: backlight <0-100>");
        }
    } else if (c.equalsIgnoreCase("vol up")) {
        usb_send_volume_up();
        led_ring_show_volume(50);
        Serial.println("[CMD] Sent USB HID Volume Up");
    } else if (c.equalsIgnoreCase("vol down")) {
        usb_send_volume_down();
        led_ring_show_volume(50);
        Serial.println("[CMD] Sent USB HID Volume Down");
    } else if (c.equalsIgnoreCase("restart") || c.equalsIgnoreCase("reboot")) {
        Serial.println("[CMD] Rebooting device...");
        delay(100);
        ESP.restart();
    } else {
        Serial.printf("[CMD] Unknown command: '%s'. Type 'help' for available commands.\n", c.c_str());
    }
}

void protocol_update() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            rx_line.trim();
            if (rx_line.length() > 0) {
                if (rx_line.startsWith("{")) {
                    // JSON packet from PC companion
                    JsonDocument doc;
                    DeserializationError err = deserializeJson(doc, rx_line);
                    if (!err) {
                        const char *type = doc["type"];
                        if (type) {
                            if (strcmp(type, "clock") == 0) {
                                int h = doc["hour"] | 0;
                                int m = doc["minute"] | 0;
                                int s = doc["sec"] | 0;
                                const char *d = doc["date"] | "";
                                const char *day = doc["day"] | "";
                                page_clock_update_time(h, m, s);
                                page_clock_update_date(d, day);
                            } else if (strcmp(type, "media") == 0) {
                                const char *title = doc["title"] | "";
                                const char *artist = doc["artist"] | "";
                                bool playing = doc["playing"] | false;
                                uint32_t pos = doc["pos"] | 0;
                                uint32_t dur = doc["dur"] | 0;
                                page_media_update(title, artist, playing, pos, dur);

                                if (doc["r"].is<uint8_t>() && doc["g"].is<uint8_t>() && doc["b"].is<uint8_t>()) {
                                    uint8_t r = doc["r"];
                                    uint8_t g = doc["g"];
                                    uint8_t b = doc["b"];
                                    page_media_set_accent_color(r, g, b);
                                    led_ring_set_accent_color(r, g, b);
                                    led_ring_set_mode(playing ? LED_MODE_MUSIC_ACCENT : LED_MODE_IDLE_BREATHING);
                                }
                            } else if (strcmp(type, "hw") == 0) {
                                uint8_t cpu = doc["cpu"] | 0;
                                uint8_t ram = doc["ram"] | 0;
                                float down = doc["down"] | 0.0f;
                                float up = doc["up"] | 0.0f;
                                page_hardware_update(cpu, ram, down, up);
                            } else if (strcmp(type, "teams") == 0) {
                                bool meeting = doc["meeting"] | false;
                                bool muted = doc["muted"] | true;
                                bool hand = doc["hand"] | false;
                                page_teams_update(meeting, muted, hand);
                            } else if (strcmp(type, "lock") == 0) {
                                bool locked = doc["locked"] | false;
                                if (locked) {
                                    ui_set_page(0); // Jump to Clock/Screensaver on PC lock
                                }
                            }
                        }
                    }
                } else {
                    // Plain text interactive command from Serial Monitor
                    handle_command(rx_line);
                }
                rx_line = "";
            }
        } else {
            rx_line += c;
        }
    }
}

