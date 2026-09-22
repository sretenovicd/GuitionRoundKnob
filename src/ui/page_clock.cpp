#include "page_clock.h"

static lv_obj_t *clock_page = NULL;
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *lbl_sec = NULL;
static lv_obj_t *lbl_date = NULL;
static lv_obj_t *arc_seconds = NULL;

void page_clock_create(lv_obj_t *parent) {
    clock_page = parent;

    // Dark sleek background
    lv_obj_set_style_bg_color(clock_page, lv_color_hex(0x0A0E17), 0);

    // Outer subtle circular border
    lv_obj_t *outer_ring = lv_arc_create(clock_page);
    lv_obj_set_size(outer_ring, 340, 340);
    lv_obj_align(outer_ring, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_angles(outer_ring, 0, 360);
    lv_arc_set_bg_angles(outer_ring, 0, 360);
    lv_obj_remove_style(outer_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(outer_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(outer_ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(outer_ring, lv_color_hex(0x1F2A38), LV_PART_MAIN);

    // Dynamic Second Arc
    arc_seconds = lv_arc_create(clock_page);
    lv_obj_set_size(arc_seconds, 330, 330);
    lv_obj_align(arc_seconds, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_range(arc_seconds, 0, 60);
    lv_arc_set_value(arc_seconds, 0);
    lv_arc_set_bg_angles(arc_seconds, 0, 360);
    lv_obj_remove_style(arc_seconds, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_seconds, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_seconds, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_seconds, lv_color_hex(0x151E28), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_seconds, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_seconds, lv_color_hex(0x00D2FF), LV_PART_INDICATOR);

    // Large Digital Time Label (HH:MM)
    lbl_time = lv_label_create(clock_page);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, -20);

    // Seconds Label (:SS)
    lbl_sec = lv_label_create(clock_page);
    lv_label_set_text(lbl_sec, ":00");
    lv_obj_set_style_text_font(lbl_sec, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_sec, lv_color_hex(0x00D2FF), 0);
    lv_obj_align_to(lbl_sec, lbl_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 5, -8);

    // Date Label
    lbl_date = lv_label_create(clock_page);
    lv_label_set_text(lbl_date, "Waiting for PC link...");
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0x8A99AD), 0);
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, 35);
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
