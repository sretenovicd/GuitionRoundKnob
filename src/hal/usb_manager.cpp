#include "usb_manager.h"
#include "USB.h"
#include "USBHIDConsumerControl.h"

static USBHIDConsumerControl ConsumerControl;

void usb_manager_init() {
    ConsumerControl.begin();
    USB.begin();
}

void usb_send_volume_up() {
    ConsumerControl.press(CONSUMER_CONTROL_VOLUME_INCREMENT);
    ConsumerControl.release();
}

void usb_send_volume_down() {
    ConsumerControl.press(CONSUMER_CONTROL_VOLUME_DECREMENT);
    ConsumerControl.release();
}

void usb_send_play_pause() {
    ConsumerControl.press(CONSUMER_CONTROL_PLAY_PAUSE);
    ConsumerControl.release();
}

void usb_send_next_track() {
    ConsumerControl.press(CONSUMER_CONTROL_SCAN_NEXT);
    ConsumerControl.release();
}

void usb_send_prev_track() {
    ConsumerControl.press(CONSUMER_CONTROL_SCAN_PREVIOUS);
    ConsumerControl.release();
}

void usb_send_mute() {
    ConsumerControl.press(CONSUMER_CONTROL_MUTE);
    ConsumerControl.release();
}
