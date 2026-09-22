# Guition JC3636K718C Smart Knob Controller

Custom firmware and PC companion service for the **Guition JC3636K718C** round smart knob desktop controller (ESP32-S3, 1.85" 360x360 round TFT display, CST816 capacitive touch, mechanical pulse rotary encoder, and 13x WS2812 addressable RGB LED ring).

---

## Features

- **Multi-Page Touch Carousel (LVGL 8.4)**:
  - **Clock / Screensaver Page**: Real-time clock with circular seconds arc and date display.
  - **Media Page**: Track title, artist, progress bar, and media controls.
  - **Hardware Monitor Page**: Real-time CPU %, RAM %, and Network Download/Upload speeds with dynamic color gradients.
  - **Hardware Backlight Control**: Rotate the knob on the HW page to adjust display backlight from 10% to 100% in discrete 10% steps, with live on-screen percentage indicator.
  - **Teams Page**: Mic mute and hand-raise meeting controls.
- **Asymmetric Rotary Encoder Debouncing**:
  - Mutual lockout debounce algorithm filtering contact chatter (80ms same-pin) and cross-talk noise (150ms opposite-pin).
  - Configurable live via serial console (`knob lockout <ms>`, `knob opplockout <ms>`).
- **USB Composite Mode (CDC + HID Consumer)**:
  - Sends native Windows Volume Up / Volume Down keystrokes without extra software.
  - Serial CDC interface for telemetry and debug monitoring.
- **WS2812 RGB LED Ring Feedback**:
  - 13 addressable LEDs provide volume, brightness gauge, meeting status, and ambient breathing feedback.
- **PC Companion Service**:
  - Lightweight Python telemetry client (requires no admin privileges).
  - Automatic port resolution bypassing Windows Bluetooth COM conflicts.
  - Workstation lock detection (auto-switches to screensaver when PC locks).

---

## Hardware Pinout Reference

| Peripheral | Function | GPIO Pin | Notes |
| :--- | :--- | :--- | :--- |
| **ST77916 Display** | SCK (Clock) | `GPIO 11` | QSPI Bus Mode |
| | CS (Chip Select) | `GPIO 12` | |
| | D0, D1, D2, D3 | `GPIO 13, 14, 15, 16` | Quad SPI data lines |
| | RST (Reset) | `GPIO 17` | |
| | BLK (Backlight) | `GPIO 21` | LEDC PWM Brightness Control |
| **CST816 Touch** | SDA / SCL | `GPIO 9 / GPIO 10` | 400kHz I2C Bus |
| | INT / RST | `GPIO 7 / GPIO 8` | Touch interrupt and hardware reset |
| **Rotary Knob** | Left Rotation | `GPIO 2` | Pulses LOW on CCW rotation |
| | Right Rotation | `GPIO 1` | Pulses LOW on CW rotation |
| **LED Ring** | Data | `GPIO 0` | 13x WS2812 (ESP32-S3 BOOT pin) |
| **Audio DAC** | BCK / WS / DO | `GPIO 3 / GPIO 45 / GPIO 42` | I2S PCM5100A |
| | MUTE | `GPIO 46` | Must be held HIGH to unmute |
| **Microphone** | Data / Clock | `GPIO 4 / GPIO 5` | PDM Microphone |
| **Battery ADC** | Voltage divider | `GPIO 6` | 10k/10k divider |

---

## Project Structure

```
GuitionRoundKnob/
├── platformio.ini           # PlatformIO configuration & build flags
├── partitions.csv           # 16MB dual-app partition table
├── include/
│   └── lv_conf.h            # LVGL configuration (16-bit color, swap enabled)
├── src/
│   ├── config.h             # Hardware GPIO assignments
│   ├── main.cpp             # Setup & main scheduler loop
│   ├── hal/
│   │   ├── display_st77916  # Native esp_lcd QSPI driver & PWM backlight
│   │   ├── touch_cst816     # CST816 I2C touch & swipe gesture engine
│   │   ├── knob_pulses      # Encoder ISR with asymmetric dual-lockout
│   │   ├── led_ring         # WS2812 LED priority arbitrator
│   │   └── usb_manager      # TinyUSB composite CDC + HID consumer
│   ├── protocol/
│   │   └── serial_protocol  # Telemetry parser & debug command console
│   └── ui/
│       ├── ui_manager       # 4-page carousel switcher & knob router
│       ├── page_clock       # Clock / Screensaver
│       ├── page_media       # Media player
│       ├── page_hardware    # CPU / RAM / Network & Backlight display
│       └── page_teams       # Teams meeting controls
├── pc_companion/
│   ├── companion.py         # Python host service
│   └── requirements.txt     # pyserial, psutil
└── README.md
```

---

## Flashing & Uploading

1. Connect the knob to your PC via USB-C.
2. If flashing for the first time or entering download mode:
   - Insert a SIM ejector tool into the pinhole switch next to the power slider.
   - Hold the pin pressed and slide power **ON**.
   - Release the pin.
3. Build and upload using PlatformIO:
   ```bash
   pio run -t upload
   ```
4. Open the Serial Monitor (115200 baud):
   ```bash
   pio device monitor
   ```

---

## Running PC Companion

```bash
pip install -r pc_companion/requirements.txt
python pc_companion/companion.py
```
