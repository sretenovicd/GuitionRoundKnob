#include "page_clock.h"
#include "../hal/led_ring.h"
#include "../config.h"

// -------------------------------------------------------------
// Submode State
// -------------------------------------------------------------
enum ClockSubmode {
    SUBMODE_NORMAL = 0,
    SUBMODE_STOPWATCH = 1,
    SUBMODE_POMODORO = 2
};

static ClockSubmode current_submode = SUBMODE_NORMAL;

// Containers for views
static lv_obj_t *clock_page = NULL;
static lv_obj_t *cont_normal = NULL;
static lv_obj_t *cont_stopwatch = NULL;
static lv_obj_t *cont_pomodoro = NULL;

// -------------------------------------------------------------
// Normal Clock View Objects
// -------------------------------------------------------------
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *lbl_sec = NULL;
static lv_obj_t *lbl_date = NULL;
static lv_obj_t *arc_seconds = NULL;
static lv_obj_t *lbl_badge = NULL;
static lv_obj_t *btn_goto_sw = NULL;
static lv_obj_t *btn_goto_pom = NULL;

// -------------------------------------------------------------
// Stopwatch State & Objects
// -------------------------------------------------------------
static lv_obj_t *lbl_sw_time = NULL;
static lv_obj_t *lbl_sw_sub = NULL;
static lv_obj_t *lbl_sw_status = NULL;
static lv_obj_t *btn_sw_start = NULL;
static lv_obj_t *lbl_sw_start_txt = NULL;
static lv_obj_t *btn_sw_clear = NULL;
static lv_obj_t *btn_sw_exit = NULL;

static bool sw_running = false;
static uint32_t sw_elapsed_ms = 0;
static unsigned long sw_last_tick_ms = 0;
static uint32_t sw_last_10s_block = 0;
static uint8_t sw_color_idx = 0;

static const uint32_t SW_COLORS[] = {
    0x00E5FF, // Vivid Cyan
    0xFF007F, // Vivid Pink / Magenta
    0x00FF66, // Bright Lime / Green
    0xFFB300, // Amber / Gold
    0x9D4EDD, // Electric Purple
    0xFF5722, // Orange Red
    0x00E676, // Spring Green
    0x00B0FF  // Sky Blue
};
#define NUM_SW_COLORS (sizeof(SW_COLORS) / sizeof(SW_COLORS[0]))

// -------------------------------------------------------------
// Pomodoro State & Objects
// -------------------------------------------------------------
static lv_obj_t *arc_pomodoro = NULL;
static lv_obj_t *lbl_pom_time = NULL;
static lv_obj_t *lbl_pom_status = NULL;
static lv_obj_t *btn_pom_add1 = NULL;
static lv_obj_t *btn_pom_add5 = NULL;
static lv_obj_t *btn_pom_add10 = NULL;
static lv_obj_t *btn_pom_start = NULL;
static lv_obj_t *lbl_pom_start_txt = NULL;
static lv_obj_t *btn_pom_clear = NULL;
static lv_obj_t *btn_pom_exit = NULL;

static bool pom_running = false;
static int pom_remaining_seconds = 0; // Starts at 0:00 as requested
static int pom_total_seconds = 0;
static unsigned long pom_last_tick_ms = 0;
static bool pom_completed_alarm = false;

static lv_timer_t *clock_submode_timer = NULL;

// -------------------------------------------------------------
// Helpers & Submode Switching
// -------------------------------------------------------------
static void calculate_pomodoro_color(float progress, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (progress >= 0.5f) {
        float t = (progress - 0.5f) / 0.5f; // 0.0 (Amber) -> 1.0 (Green)
        *r = (uint8_t)(255 * (1.0f - t) + 0 * t);
        *g = (uint8_t)(190 * (1.0f - t) + 230 * t);
        *b = (uint8_t)(0 * (1.0f - t) + 160 * t);
    } else {
        float t = progress / 0.5f; // 0.0 (Red) -> 1.0 (Amber)
        *r = 255;
        *g = (uint8_t)(30 * (1.0f - t) + 190 * t);
        *b = (uint8_t)(30 * (1.0f - t) + 0 * t);
    }
}

static void update_pomodoro_ui();

static void set_submode(ClockSubmode submode) {
    current_submode = submode;

    if (submode == SUBMODE_NORMAL) {
        lv_obj_clear_flag(cont_normal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_stopwatch, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_pomodoro, LV_OBJ_FLAG_HIDDEN);
        if (!pom_running) {
            led_ring_set_pomodoro_state(false, 0, 0, 0, 0);
        }
    } else if (submode == SUBMODE_STOPWATCH) {
        lv_obj_add_flag(cont_normal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(cont_stopwatch, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_pomodoro, LV_OBJ_FLAG_HIDDEN);
    } else if (submode == SUBMODE_POMODORO) {
        lv_obj_add_flag(cont_normal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_stopwatch, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(cont_pomodoro, LV_OBJ_FLAG_HIDDEN);
        update_pomodoro_ui();
    }
}

bool page_clock_is_in_submode() {
    return current_submode != SUBMODE_NORMAL;
}

bool page_clock_is_in_pomodoro() {
    return current_submode == SUBMODE_POMODORO;
}

void page_clock_return_to_normal() {
    set_submode(SUBMODE_NORMAL);
}

// -------------------------------------------------------------
// UI Updates for Stopwatch
// -------------------------------------------------------------
static void update_stopwatch_ui() {
    if (!lbl_sw_time || !lbl_sw_sub) return;

    uint32_t total_sec = sw_elapsed_ms / 1000;
    uint32_t mins = total_sec / 60;
    uint32_t secs = total_sec % 60;
    uint32_t hundredths = (sw_elapsed_ms % 1000) / 10;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)mins, (unsigned long)secs);
    lv_label_set_text(lbl_sw_time, buf);

    snprintf(buf, sizeof(buf), ".%02lu", (unsigned long)hundredths);
    lv_label_set_text(lbl_sw_sub, buf);

    if (sw_running) {
        lv_label_set_text(lbl_sw_status, "RUNNING");
        lv_obj_set_style_text_color(lbl_sw_status, lv_color_hex(0x00FF66), 0);
        lv_label_set_text(lbl_sw_start_txt, "PAUSE");
        lv_obj_set_style_bg_color(btn_sw_start, lv_color_hex(0xE67E00), 0);
    } else {
        if (sw_elapsed_ms == 0) {
            lv_label_set_text(lbl_sw_status, "READY");
            lv_obj_set_style_text_color(lbl_sw_status, lv_color_hex(0x8A99AD), 0);
        } else {
            lv_label_set_text(lbl_sw_status, "PAUSED");
            lv_obj_set_style_text_color(lbl_sw_status, lv_color_hex(0xFFBE00), 0);
        }
        lv_label_set_text(lbl_sw_start_txt, "START");
        lv_obj_set_style_bg_color(btn_sw_start, lv_color_hex(0x00B060), 0);
    }
}

// -------------------------------------------------------------
// UI Updates for Pomodoro
// -------------------------------------------------------------
static void update_pomodoro_ui() {
    if (!lbl_pom_time || !arc_pomodoro) return;

    int mins = pom_remaining_seconds / 60;
    int secs = pom_remaining_seconds % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    lv_label_set_text(lbl_pom_time, buf);

    float progress = 0.0f;
    if (pom_total_seconds > 0) {
        progress = (float)pom_remaining_seconds / (float)pom_total_seconds;
        if (progress > 1.0f) progress = 1.0f;
        if (progress < 0.0f) progress = 0.0f;
    }

    uint8_t r = 0, g = 230, b = 160;
    calculate_pomodoro_color(progress, &r, &g, &b);

    // Update Arc: full circle (1000) reduced proportionally to 0
    int16_t arc_val = (int16_t)(progress * 1000.0f);
    lv_arc_set_value(arc_pomodoro, arc_val);
    lv_obj_set_style_arc_color(arc_pomodoro, lv_color_make(r, g, b), LV_PART_INDICATOR);

    // Update RGB Ring mirror
    uint8_t pct = (uint8_t)(progress * 100.0f);
    led_ring_set_pomodoro_state(true, pct, r, g, b);

    if (pom_completed_alarm) {
        lv_label_set_text(lbl_pom_status, "TIME'S UP!");
        lv_obj_set_style_text_color(lbl_pom_status, lv_color_hex(0xFF2A2A), 0);
    } else if (pom_running) {
        lv_label_set_text(lbl_pom_status, "COUNTING DOWN");
        lv_obj_set_style_text_color(lbl_pom_status, lv_color_make(r, g, b), 0);
        lv_label_set_text(lbl_pom_start_txt, "PAUSE");
        lv_obj_set_style_bg_color(btn_pom_start, lv_color_hex(0xE67E00), 0);
    } else {
        if (pom_remaining_seconds == 0) {
            lv_label_set_text(lbl_pom_status, "SET TIMER");
            lv_obj_set_style_text_color(lbl_pom_status, lv_color_hex(0x8A99AD), 0);
        } else {
            lv_label_set_text(lbl_pom_status, "PAUSED");
            lv_obj_set_style_text_color(lbl_pom_status, lv_color_hex(0xFFBE00), 0);
        }
        lv_label_set_text(lbl_pom_start_txt, "START");
        lv_obj_set_style_bg_color(btn_pom_start, lv_color_hex(0x00B060), 0);
    }
}

// -------------------------------------------------------------
// Knob Adjustment for Pomodoro (+10s CW / -10s CCW)
// -------------------------------------------------------------
void page_clock_pomodoro_adjust(int delta) {
    pom_completed_alarm = false; // Reset alarm on adjustment

    if (delta > 0) {
        int add = delta * 10;
        pom_remaining_seconds += add;
        pom_total_seconds += add;
    } else if (delta < 0) {
        int sub = (-delta) * 10;
        if (pom_remaining_seconds > sub) {
            pom_remaining_seconds -= sub;
            if (pom_total_seconds >= sub) {
                pom_total_seconds -= sub;
            } else {
                pom_total_seconds = pom_remaining_seconds;
            }
        } else {
            pom_remaining_seconds = 0;
            pom_total_seconds = 0;
            pom_running = false;
        }
    }

    update_pomodoro_ui();
}

// -------------------------------------------------------------
// Periodic Timer Callback (LVGL timer, 50ms)
// -------------------------------------------------------------
static void clock_submode_timer_cb(lv_timer_t *timer) {
    unsigned long now = millis();

    // 1. Process Stopwatch
    if (sw_running) {
        sw_elapsed_ms += (now - sw_last_tick_ms);
        sw_last_tick_ms = now;

        // Check 10s blink milestone
        uint32_t current_10s_block = sw_elapsed_ms / 10000;
        if (current_10s_block > sw_last_10s_block) {
            sw_last_10s_block = current_10s_block;
            uint32_t col = SW_COLORS[sw_color_idx % NUM_SW_COLORS];
            sw_color_idx++;
            uint8_t r = (col >> 16) & 0xFF;
            uint8_t g = (col >> 8) & 0xFF;
            uint8_t b = col & 0xFF;
            led_ring_flash_stopwatch(r, g, b, 350);
        }

        if (current_submode == SUBMODE_STOPWATCH) {
            update_stopwatch_ui();
        }
    }

    // 2. Process Pomodoro
    if (pom_running) {
        if (now - pom_last_tick_ms >= 1000) {
            int elapsed_sec = (int)((now - pom_last_tick_ms) / 1000);
            pom_last_tick_ms += elapsed_sec * 1000;

            if (pom_remaining_seconds > elapsed_sec) {
                pom_remaining_seconds -= elapsed_sec;
            } else {
                // Completed!
                pom_remaining_seconds = 0;
                pom_running = false;
                pom_completed_alarm = true;
                led_ring_trigger_alarm(4000); // 4s pulsing red/gold
            }

            if (current_submode == SUBMODE_POMODORO) {
                update_pomodoro_ui();
            } else {
                // Still update background ring mirror if on clock page
                float progress = (pom_total_seconds > 0) ? ((float)pom_remaining_seconds / (float)pom_total_seconds) : 0.0f;
                uint8_t r = 0, g = 230, b = 160;
                calculate_pomodoro_color(progress, &r, &g, &b);
                led_ring_set_pomodoro_state(pom_remaining_seconds > 0, (uint8_t)(progress * 100.0f), r, g, b);
            }
        }
    }

    // 3. Update Background Status Badge on Normal Clock View
    if (current_submode == SUBMODE_NORMAL && lbl_badge) {
        char badge_buf[64] = "";
        if (sw_running && pom_running) {
            uint32_t sw_sec = sw_elapsed_ms / 1000;
            int pom_mins = pom_remaining_seconds / 60;
            int pom_secs = pom_remaining_seconds % 60;
            snprintf(badge_buf, sizeof(badge_buf), "SW: %02lu:%02lu  |  POM: %02d:%02d",
                     (unsigned long)(sw_sec / 60), (unsigned long)(sw_sec % 60),
                     pom_mins, pom_secs);
        } else if (sw_running) {
            uint32_t sw_sec = sw_elapsed_ms / 1000;
            snprintf(badge_buf, sizeof(badge_buf), "STOPWATCH RUNNING: %02lu:%02lu",
                     (unsigned long)(sw_sec / 60), (unsigned long)(sw_sec % 60));
        } else if (pom_running) {
            int pom_mins = pom_remaining_seconds / 60;
            int pom_secs = pom_remaining_seconds % 60;
            snprintf(badge_buf, sizeof(badge_buf), "POMODORO: %02d:%02d", pom_mins, pom_secs);
        } else if (pom_completed_alarm) {
            snprintf(badge_buf, sizeof(badge_buf), "POMODORO: TIME'S UP!");
        }

        lv_label_set_text(lbl_badge, badge_buf);
    }
}

// -------------------------------------------------------------
// Button Event Handlers
// -------------------------------------------------------------
static void on_btn_click(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);

    // Navigation from Clock View
    if (target == btn_goto_sw) {
        set_submode(SUBMODE_STOPWATCH);
        update_stopwatch_ui();
        return;
    }
    if (target == btn_goto_pom) {
        set_submode(SUBMODE_POMODORO);
        update_pomodoro_ui();
        return;
    }

    // Stopwatch Controls
    if (target == btn_sw_start) {
        if (!sw_running) {
            sw_running = true;
            sw_last_tick_ms = millis();
        } else {
            sw_running = false;
        }
        update_stopwatch_ui();
        return;
    }
    if (target == btn_sw_clear) {
        sw_running = false;
        sw_elapsed_ms = 0;
        sw_last_10s_block = 0;
        update_stopwatch_ui();
        return;
    }
    if (target == btn_sw_exit) {
        set_submode(SUBMODE_NORMAL);
        return;
    }

    // Pomodoro Controls
    if (target == btn_pom_add1) {
        pom_completed_alarm = false;
        pom_remaining_seconds += 60;
        pom_total_seconds += 60;
        update_pomodoro_ui();
        return;
    }
    if (target == btn_pom_add5) {
        pom_completed_alarm = false;
        pom_remaining_seconds += 300;
        pom_total_seconds += 300;
        update_pomodoro_ui();
        return;
    }
    if (target == btn_pom_add10) {
        pom_completed_alarm = false;
        pom_remaining_seconds += 600;
        pom_total_seconds += 600;
        update_pomodoro_ui();
        return;
    }
    if (target == btn_pom_start) {
        if (!pom_running) {
            if (pom_remaining_seconds > 0) {
                pom_running = true;
                pom_completed_alarm = false;
                pom_last_tick_ms = millis();
            }
        } else {
            pom_running = false;
        }
        update_pomodoro_ui();
        return;
    }
    if (target == btn_pom_clear) {
        pom_running = false;
        pom_completed_alarm = false;
        pom_remaining_seconds = 0;
        pom_total_seconds = 0;
        update_pomodoro_ui();
        return;
    }
    if (target == btn_pom_exit) {
        set_submode(SUBMODE_NORMAL);
        return;
    }
}

// -------------------------------------------------------------
// UI Construction
// -------------------------------------------------------------
void page_clock_create(lv_obj_t *parent) {
    clock_page = parent;
    lv_obj_set_style_bg_color(clock_page, lv_color_hex(0x0A0E17), 0);

    // =========================================================
    // 1. NORMAL CLOCK VIEW CONTAINER
    // =========================================================
    cont_normal = lv_obj_create(clock_page);
    lv_obj_set_size(cont_normal, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_align(cont_normal, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont_normal, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_normal, 0, 0);
    lv_obj_clear_flag(cont_normal, LV_OBJ_FLAG_SCROLLABLE);

    // Outer subtle circular border
    lv_obj_t *outer_ring = lv_arc_create(cont_normal);
    lv_obj_set_size(outer_ring, 340, 340);
    lv_obj_align(outer_ring, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_angles(outer_ring, 0, 360);
    lv_arc_set_bg_angles(outer_ring, 0, 360);
    lv_obj_remove_style(outer_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(outer_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(outer_ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(outer_ring, lv_color_hex(0x1F2A38), LV_PART_MAIN);

    // Dynamic Second Arc
    arc_seconds = lv_arc_create(cont_normal);
    lv_obj_set_size(arc_seconds, 330, 330);
    lv_obj_align(arc_seconds, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(arc_seconds, 270);
    lv_arc_set_bg_angles(arc_seconds, 0, 360);
    lv_arc_set_range(arc_seconds, 0, 60);
    lv_arc_set_value(arc_seconds, 0);
    lv_obj_remove_style(arc_seconds, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_seconds, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_seconds, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_seconds, lv_color_hex(0x151E28), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_seconds, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_seconds, lv_color_hex(0x00D2FF), LV_PART_INDICATOR);

    // Large Digital Time Label (HH:MM)
    lbl_time = lv_label_create(cont_normal);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, -40);

    // Seconds Label (:SS)
    lbl_sec = lv_label_create(cont_normal);
    lv_label_set_text(lbl_sec, ":00");
    lv_obj_set_style_text_font(lbl_sec, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_sec, lv_color_hex(0x00D2FF), 0);
    lv_obj_align_to(lbl_sec, lbl_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, -8);

    // Date Label
    lbl_date = lv_label_create(cont_normal);
    lv_label_set_text(lbl_date, "Waiting for PC link...");
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0x8A99AD), 0);
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, 15);

    // Background running badge (displays when Stopwatch or Pomodoro running)
    lbl_badge = lv_label_create(cont_normal);
    lv_label_set_text(lbl_badge, "");
    lv_obj_set_style_text_font(lbl_badge, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_badge, lv_color_hex(0x00FFB0), 0);
    lv_obj_align(lbl_badge, LV_ALIGN_CENTER, 0, 48);

    // Stopwatch Launch Button
    btn_goto_sw = lv_btn_create(cont_normal);
    lv_obj_set_size(btn_goto_sw, 116, 38);
    lv_obj_align(btn_goto_sw, LV_ALIGN_CENTER, -68, 92);
    lv_obj_set_style_radius(btn_goto_sw, 19, 0);
    lv_obj_set_style_bg_color(btn_goto_sw, lv_color_hex(0x1B2636), 0);
    lv_obj_set_style_border_width(btn_goto_sw, 1, 0);
    lv_obj_set_style_border_color(btn_goto_sw, lv_color_hex(0x2D3E56), 0);
    lv_obj_add_event_cb(btn_goto_sw, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_btn_sw = lv_label_create(btn_goto_sw);
    lv_label_set_text(lbl_btn_sw, "Stopwatch");
    lv_obj_set_style_text_font(lbl_btn_sw, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_btn_sw, lv_color_hex(0x00D2FF), 0);
    lv_obj_center(lbl_btn_sw);

    // Pomodoro Launch Button
    btn_goto_pom = lv_btn_create(cont_normal);
    lv_obj_set_size(btn_goto_pom, 116, 38);
    lv_obj_align(btn_goto_pom, LV_ALIGN_CENTER, 68, 92);
    lv_obj_set_style_radius(btn_goto_pom, 19, 0);
    lv_obj_set_style_bg_color(btn_goto_pom, lv_color_hex(0x1B2636), 0);
    lv_obj_set_style_border_width(btn_goto_pom, 1, 0);
    lv_obj_set_style_border_color(btn_goto_pom, lv_color_hex(0x2D3E56), 0);
    lv_obj_add_event_cb(btn_goto_pom, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_btn_pom = lv_label_create(btn_goto_pom);
    lv_label_set_text(lbl_btn_pom, "Pomodoro");
    lv_obj_set_style_text_font(lbl_btn_pom, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_btn_pom, lv_color_hex(0xFF6B4A), 0);
    lv_obj_center(lbl_btn_pom);


    // =========================================================
    // 2. STOPWATCH VIEW CONTAINER
    // =========================================================
    cont_stopwatch = lv_obj_create(clock_page);
    lv_obj_set_size(cont_stopwatch, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_align(cont_stopwatch, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont_stopwatch, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_stopwatch, 0, 0);
    lv_obj_clear_flag(cont_stopwatch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont_stopwatch, LV_OBJ_FLAG_HIDDEN);

    // Decorative ring
    lv_obj_t *sw_ring = lv_arc_create(cont_stopwatch);
    lv_obj_set_size(sw_ring, 336, 336);
    lv_obj_align(sw_ring, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_angles(sw_ring, 0, 360);
    lv_arc_set_bg_angles(sw_ring, 0, 360);
    lv_obj_remove_style(sw_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(sw_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(sw_ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(sw_ring, lv_color_hex(0x1F2A38), LV_PART_MAIN);

    // Title
    lv_obj_t *lbl_sw_title = lv_label_create(cont_stopwatch);
    lv_label_set_text(lbl_sw_title, "STOPWATCH");
    lv_obj_set_style_text_font(lbl_sw_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sw_title, lv_color_hex(0x00D2FF), 0);
    lv_obj_align(lbl_sw_title, LV_ALIGN_CENTER, 0, -115);

    // Main Elapsed Time (MM:SS)
    lbl_sw_time = lv_label_create(cont_stopwatch);
    lv_label_set_text(lbl_sw_time, "00:00");
    lv_obj_set_style_text_font(lbl_sw_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_sw_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_sw_time, LV_ALIGN_CENTER, -25, -45);

    // Sub-seconds (.00)
    lbl_sw_sub = lv_label_create(cont_stopwatch);
    lv_label_set_text(lbl_sw_sub, ".00");
    lv_obj_set_style_text_font(lbl_sw_sub, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_sw_sub, lv_color_hex(0x00E5FF), 0);
    lv_obj_align_to(lbl_sw_sub, lbl_sw_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, -8);

    // Status Label
    lbl_sw_status = lv_label_create(cont_stopwatch);
    lv_label_set_text(lbl_sw_status, "READY");
    lv_obj_set_style_text_font(lbl_sw_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sw_status, lv_color_hex(0x8A99AD), 0);
    lv_obj_align(lbl_sw_status, LV_ALIGN_CENTER, 0, 10);

    // Start / Pause Button
    btn_sw_start = lv_btn_create(cont_stopwatch);
    lv_obj_set_size(btn_sw_start, 106, 40);
    lv_obj_align(btn_sw_start, LV_ALIGN_CENTER, -62, 65);
    lv_obj_set_style_radius(btn_sw_start, 20, 0);
    lv_obj_set_style_bg_color(btn_sw_start, lv_color_hex(0x00B060), 0);
    lv_obj_add_event_cb(btn_sw_start, on_btn_click, LV_EVENT_CLICKED, NULL);

    lbl_sw_start_txt = lv_label_create(btn_sw_start);
    lv_label_set_text(lbl_sw_start_txt, "START");
    lv_obj_set_style_text_font(lbl_sw_start_txt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sw_start_txt, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_sw_start_txt);

    // Clear Button
    btn_sw_clear = lv_btn_create(cont_stopwatch);
    lv_obj_set_size(btn_sw_clear, 106, 40);
    lv_obj_align(btn_sw_clear, LV_ALIGN_CENTER, 62, 65);
    lv_obj_set_style_radius(btn_sw_clear, 20, 0);
    lv_obj_set_style_bg_color(btn_sw_clear, lv_color_hex(0x222C3A), 0);
    lv_obj_set_style_border_width(btn_sw_clear, 1, 0);
    lv_obj_set_style_border_color(btn_sw_clear, lv_color_hex(0x3B4C63), 0);
    lv_obj_add_event_cb(btn_sw_clear, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_sw_clear_txt = lv_label_create(btn_sw_clear);
    lv_label_set_text(lbl_sw_clear_txt, "CLEAR");
    lv_obj_set_style_text_font(lbl_sw_clear_txt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sw_clear_txt, lv_color_hex(0xD0D7DE), 0);
    lv_obj_center(lbl_sw_clear_txt);

    // Exit Button
    btn_sw_exit = lv_btn_create(cont_stopwatch);
    lv_obj_set_size(btn_sw_exit, 110, 32);
    lv_obj_align(btn_sw_exit, LV_ALIGN_CENTER, 0, 118);
    lv_obj_set_style_radius(btn_sw_exit, 16, 0);
    lv_obj_set_style_bg_color(btn_sw_exit, lv_color_hex(0x151D29), 0);
    lv_obj_set_style_border_width(btn_sw_exit, 1, 0);
    lv_obj_set_style_border_color(btn_sw_exit, lv_color_hex(0x28374D), 0);
    lv_obj_add_event_cb(btn_sw_exit, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_sw_exit_txt = lv_label_create(btn_sw_exit);
    lv_label_set_text(lbl_sw_exit_txt, "EXIT");
    lv_obj_set_style_text_font(lbl_sw_exit_txt, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_sw_exit_txt, lv_color_hex(0x8A99AD), 0);
    lv_obj_center(lbl_sw_exit_txt);


    // =========================================================
    // 3. POMODORO VIEW CONTAINER
    // =========================================================
    cont_pomodoro = lv_obj_create(clock_page);
    lv_obj_set_size(cont_pomodoro, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_align(cont_pomodoro, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont_pomodoro, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_pomodoro, 0, 0);
    lv_obj_clear_flag(cont_pomodoro, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont_pomodoro, LV_OBJ_FLAG_HIDDEN);

    // Pomodoro Dynamic Circular Arc (Full circle reduced proportionally to 0)
    arc_pomodoro = lv_arc_create(cont_pomodoro);
    lv_obj_set_size(arc_pomodoro, 334, 334);
    lv_obj_align(arc_pomodoro, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(arc_pomodoro, 270); // 12 o'clock start
    lv_arc_set_bg_angles(arc_pomodoro, 0, 360);
    lv_arc_set_range(arc_pomodoro, 0, 1000);
    lv_arc_set_value(arc_pomodoro, 0);
    lv_obj_remove_style(arc_pomodoro, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_pomodoro, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_pomodoro, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_pomodoro, lv_color_hex(0x151E28), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_pomodoro, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_pomodoro, lv_color_hex(0x00E6A0), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_pomodoro, true, LV_PART_INDICATOR);

    // Title
    lv_obj_t *lbl_pom_title = lv_label_create(cont_pomodoro);
    lv_label_set_text(lbl_pom_title, "POMODORO");
    lv_obj_set_style_text_font(lbl_pom_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pom_title, lv_color_hex(0xFF6B4A), 0);
    lv_obj_align(lbl_pom_title, LV_ALIGN_CENTER, 0, -118);

    // Large Digital Countdown (MM:SS)
    lbl_pom_time = lv_label_create(cont_pomodoro);
    lv_label_set_text(lbl_pom_time, "00:00");
    lv_obj_set_style_text_font(lbl_pom_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_pom_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_pom_time, LV_ALIGN_CENTER, 0, -60);

    // Status / Mode feedback
    lbl_pom_status = lv_label_create(cont_pomodoro);
    lv_label_set_text(lbl_pom_status, "SET TIMER");
    lv_obj_set_style_text_font(lbl_pom_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pom_status, lv_color_hex(0x8A99AD), 0);
    lv_obj_align(lbl_pom_status, LV_ALIGN_CENTER, 0, -15);

    // Quick-add buttons: +1, +5, +10
    btn_pom_add1 = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_add1, 56, 32);
    lv_obj_align(btn_pom_add1, LV_ALIGN_CENTER, -68, 24);
    lv_obj_set_style_radius(btn_pom_add1, 16, 0);
    lv_obj_set_style_bg_color(btn_pom_add1, lv_color_hex(0x1B2636), 0);
    lv_obj_set_style_border_width(btn_pom_add1, 1, 0);
    lv_obj_set_style_border_color(btn_pom_add1, lv_color_hex(0x2D3E56), 0);
    lv_obj_add_event_cb(btn_pom_add1, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_add1 = lv_label_create(btn_pom_add1);
    lv_label_set_text(lbl_add1, "+1m");
    lv_obj_set_style_text_font(lbl_add1, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_add1, lv_color_hex(0x00E5FF), 0);
    lv_obj_center(lbl_add1);

    btn_pom_add5 = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_add5, 56, 32);
    lv_obj_align(btn_pom_add5, LV_ALIGN_CENTER, 0, 24);
    lv_obj_set_style_radius(btn_pom_add5, 16, 0);
    lv_obj_set_style_bg_color(btn_pom_add5, lv_color_hex(0x1B2636), 0);
    lv_obj_set_style_border_width(btn_pom_add5, 1, 0);
    lv_obj_set_style_border_color(btn_pom_add5, lv_color_hex(0x2D3E56), 0);
    lv_obj_add_event_cb(btn_pom_add5, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_add5 = lv_label_create(btn_pom_add5);
    lv_label_set_text(lbl_add5, "+5m");
    lv_obj_set_style_text_font(lbl_add5, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_add5, lv_color_hex(0x00E5FF), 0);
    lv_obj_center(lbl_add5);

    btn_pom_add10 = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_add10, 56, 32);
    lv_obj_align(btn_pom_add10, LV_ALIGN_CENTER, 68, 24);
    lv_obj_set_style_radius(btn_pom_add10, 16, 0);
    lv_obj_set_style_bg_color(btn_pom_add10, lv_color_hex(0x1B2636), 0);
    lv_obj_set_style_border_width(btn_pom_add10, 1, 0);
    lv_obj_set_style_border_color(btn_pom_add10, lv_color_hex(0x2D3E56), 0);
    lv_obj_add_event_cb(btn_pom_add10, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_add10 = lv_label_create(btn_pom_add10);
    lv_label_set_text(lbl_add10, "+10m");
    lv_obj_set_style_text_font(lbl_add10, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_add10, lv_color_hex(0x00E5FF), 0);
    lv_obj_center(lbl_add10);

    // Start / Pause Button
    btn_pom_start = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_start, 106, 38);
    lv_obj_align(btn_pom_start, LV_ALIGN_CENTER, -62, 70);
    lv_obj_set_style_radius(btn_pom_start, 19, 0);
    lv_obj_set_style_bg_color(btn_pom_start, lv_color_hex(0x00B060), 0);
    lv_obj_add_event_cb(btn_pom_start, on_btn_click, LV_EVENT_CLICKED, NULL);

    lbl_pom_start_txt = lv_label_create(btn_pom_start);
    lv_label_set_text(lbl_pom_start_txt, "START");
    lv_obj_set_style_text_font(lbl_pom_start_txt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pom_start_txt, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_pom_start_txt);

    // Clear Button
    btn_pom_clear = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_clear, 106, 38);
    lv_obj_align(btn_pom_clear, LV_ALIGN_CENTER, 62, 70);
    lv_obj_set_style_radius(btn_pom_clear, 19, 0);
    lv_obj_set_style_bg_color(btn_pom_clear, lv_color_hex(0x222C3A), 0);
    lv_obj_set_style_border_width(btn_pom_clear, 1, 0);
    lv_obj_set_style_border_color(btn_pom_clear, lv_color_hex(0x3B4C63), 0);
    lv_obj_add_event_cb(btn_pom_clear, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_pom_clear_txt = lv_label_create(btn_pom_clear);
    lv_label_set_text(lbl_pom_clear_txt, "CLEAR");
    lv_obj_set_style_text_font(lbl_pom_clear_txt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pom_clear_txt, lv_color_hex(0xD0D7DE), 0);
    lv_obj_center(lbl_pom_clear_txt);

    // Exit Button
    btn_pom_exit = lv_btn_create(cont_pomodoro);
    lv_obj_set_size(btn_pom_exit, 110, 30);
    lv_obj_align(btn_pom_exit, LV_ALIGN_CENTER, 0, 118);
    lv_obj_set_style_radius(btn_pom_exit, 15, 0);
    lv_obj_set_style_bg_color(btn_pom_exit, lv_color_hex(0x151D29), 0);
    lv_obj_set_style_border_width(btn_pom_exit, 1, 0);
    lv_obj_set_style_border_color(btn_pom_exit, lv_color_hex(0x28374D), 0);
    lv_obj_add_event_cb(btn_pom_exit, on_btn_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_pom_exit_txt = lv_label_create(btn_pom_exit);
    lv_label_set_text(lbl_pom_exit_txt, "EXIT");
    lv_obj_set_style_text_font(lbl_pom_exit_txt, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_pom_exit_txt, lv_color_hex(0x8A99AD), 0);
    lv_obj_center(lbl_pom_exit_txt);


    // =========================================================
    // 4. PERIODIC TIMER
    // =========================================================
    if (!clock_submode_timer) {
        clock_submode_timer = lv_timer_create(clock_submode_timer_cb, 50, NULL);
    }
}

void page_clock_update_time(int hour, int minute, int second) {
    if (!lbl_time || !lbl_sec || !arc_seconds) return;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    lv_label_set_text(lbl_time, buf);

    snprintf(buf, sizeof(buf), ":%02d", second);
    lv_label_set_text(lbl_sec, buf);

    lv_arc_set_value(arc_seconds, second);
}

void page_clock_update_date(const char *date_str, const char *day_str) {
    if (!lbl_date) return;
    char buf[64];
    if (day_str && strlen(day_str) > 0) {
        snprintf(buf, sizeof(buf), "%s, %s", day_str, date_str);
    } else {
        snprintf(buf, sizeof(buf), "%s", date_str);
    }
    lv_label_set_text(lbl_date, buf);
}
