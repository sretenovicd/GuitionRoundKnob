#include "ui_manager.h"
#include "../config.h"
#include "../hal/display_st77916.h"
#include "../hal/touch_cst816.h"
#include "../hal/knob_pulses.h"
#include "../hal/usb_manager.h"
#include "../hal/led_ring.h"
#include "page_clock.h"
#include "page_media.h"
#include "page_hardware.h"
#include "page_teams.h"

#define NUM_PAGES 4

enum PageIndex {
    PAGE_CLOCK = 0,
    PAGE_MEDIA = 1,
    PAGE_HARDWARE = 2,
    PAGE_TEAMS = 3
};

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

static lv_obj_t *pages[NUM_PAGES];
static int current_page = 0;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    display_flush(area->x1, area->y1, area->x2, area->y2, (const uint16_t *)color_p);
    lv_disp_flush_ready(disp);
}

static void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    int16_t touchX, touchY;
    bool touched;
    TouchGesture gesture;

    if (touch_read(&touchX, &touchY, &touched, &gesture)) {
        data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        data->point.x = touchX;
        data->point.y = touchY;

        // Check horizontal swipe gestures for wrap-around page carousel
        // Suppress carousel swipe when currently in a Clock submode (Stopwatch or Pomodoro)
        bool in_submode = (current_page == PAGE_CLOCK && page_clock_is_in_submode());
        if (!in_submode) {
            if (gesture == GESTURE_SLIDE_LEFT) {
                ui_next_page();
            } else if (gesture == GESTURE_SLIDE_RIGHT) {
                ui_prev_page();
            }
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void on_knob_turn(int delta) {
    if (current_page == PAGE_CLOCK && page_clock_is_in_pomodoro()) {
        // Pomodoro Timer: knob turn adjusts time by 10s per detent (+10s CW / -10s CCW)
        page_clock_pomodoro_adjust(delta);
    } else if (current_page == PAGE_HARDWARE) {
        // While in HW page, turning knob increases/decreases background light of display in steps of 10%
        int current_bri = (int)display_get_backlight();
        int new_bri = current_bri + (delta * 10);
        if (new_bri > 100) new_bri = 100;
        if (new_bri < 0) new_bri = 0; // Can be reduced to 0% as requested

        display_set_backlight((uint8_t)new_bri);
        page_hardware_set_brightness((uint8_t)new_bri);
        led_ring_show_volume((uint8_t)new_bri); // Visual feedback on LED ring
        Serial.printf("[HW] Display Backlight: %d%%\n", new_bri);
    } else {
        // Default behavior: Media Volume Control
        if (delta > 0) {
            for (int i = 0; i < delta; i++) {
                usb_send_volume_up();
            }
        } else if (delta < 0) {
            for (int i = 0; i < -delta; i++) {
                usb_send_volume_down();
            }
        }
        led_ring_show_volume(50); // Flash volume ring indication
    }
}

void ui_init() {
    lv_init();

    // Allocate draw buffers in internal RAM for maximum QSPI framerate
    size_t buf_size = LCD_WIDTH * 40 * sizeof(lv_color_t);
    buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (!buf1) buf1 = (lv_color_t *)malloc(buf_size);
    if (!buf2) buf2 = (lv_color_t *)malloc(buf_size);

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * 40);

    // Register Display Driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Register Touch Input Device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Create Carousel Pages
    for (int i = 0; i < NUM_PAGES; i++) {
        pages[i] = lv_obj_create(lv_scr_act());
        lv_obj_set_size(pages[i], LCD_WIDTH, LCD_HEIGHT);
        lv_obj_align(pages[i], LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_border_width(pages[i], 0, 0);
        lv_obj_set_style_radius(pages[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN); // Hidden by default
    }

    // Populate Page Contents
    page_clock_create(pages[0]);
    page_media_create(pages[1]);
    page_hardware_create(pages[2]);
    page_teams_create(pages[3]);

    page_hardware_set_brightness(display_get_backlight());

    // Initial page is Page 0 (Clock / Screensaver)
    ui_set_page(0);

    // Hook knob
    knob_set_callback(on_knob_turn);
}

void ui_set_page(int page_index) {
    if (page_index < 0 || page_index >= NUM_PAGES) return;

    for (int i = 0; i < NUM_PAGES; i++) {
        if (i == page_index) {
            lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    current_page = page_index;
}

void ui_next_page() {
    int next = (current_page + 1) % NUM_PAGES;
    ui_set_page(next);
}

void ui_prev_page() {
    int prev = (current_page + NUM_PAGES - 1) % NUM_PAGES;
    ui_set_page(prev);
}

int ui_get_current_page() {
    return current_page;
}

void ui_update() {
    lv_timer_handler();
}
