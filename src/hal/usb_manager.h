#pragma once

#include <Arduino.h>

void usb_manager_init();
void usb_send_volume_up();
void usb_send_volume_down();
void usb_send_play_pause();
void usb_send_next_track();
void usb_send_prev_track();
void usb_send_mute();
