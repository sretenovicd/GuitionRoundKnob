#include "page_teams.h"
#include "../hal/led_ring.h"
#include <math.h>

#define NUM_CHRONO_TICKS 24

static lv_obj_t *teams_page = NULL;
static lv_obj_t *dial_base = NULL;
static lv_obj_t *chrono_ticks[NUM_CHRONO_TICKS];
static lv_point_t tick_pts[NUM_CHRONO_TICKS][2];

// Top Meeting Status Capsule (Image 3 & Image 4 style)
static lv_obj_t *pill_meeting = NULL;
static lv_obj_t *dot_meeting = NULL;
static lv_obj_t *lbl_meeting = NULL;

// Hero Mic Dial / Core Button (Obsidian Chrono Concentric)
static lv_obj_t *mic_outer_halo = NULL;
static lv_obj_t *btn_mic = NULL;
static lv_obj_t *lbl_mic_icon = NULL;
static lv_obj_t *lbl_mic_txt = NULL;

// Secondary Hand Capsule Button
static lv_obj_t *btn_hand = NULL;
static lv_obj_t *lbl_hand = NULL;

// Bottom status hint
static lv_obj_t *lbl_hint = NULL;

static bool teams_in_meeting = false;
static bool teams_muted = true;
static bool teams_hand = false;

static void btn_mic_cb(lv_event_t *e) {
    teams_muted = !teams_muted;
    Serial.println("{\"cmd\":\"teams_toggle_mute\"}");
    page_teams_update(teams_in_meeting, teams_muted, teams_hand);
}

static void btn_hand_cb(lv_event_t *e) {
    teams_hand = !teams_hand;
    Serial.println("{\"cmd\":\"teams_toggle_hand\"}");
    page_teams_update(teams_in_meeting, teams_muted, teams_hand);
}

void page_teams_create(lv_obj_t *parent) {
    teams_page = parent;
    lv_obj_set_style_bg_color(teams_page, lv_color_hex(0x06080D), 0); // Pure Obsidian Canvas

    // 1. Dial Base Disc (Diameter 344px)
    dial_base = lv_obj_create(teams_page);
    lv_obj_set_size(dial_base, 344, 344);
    lv_obj_center(dial_base);
    lv_obj_set_style_radius(dial_base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dial_base, lv_color_hex(0x0D111A), 0);
    lv_obj_set_style_border_width(dial_base, 1, 0);
    lv_obj_set_style_border_color(dial_base, lv_color_hex(0x1E2638), 0);
    lv_obj_clear_flag(dial_base, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Instrument Perimeter Micro-Ticks (24 precision ticks)
    for (int t = 0; t < NUM_CHRONO_TICKS; t++) {
        float angle_rad = ((float)t * (360.0f / (float)NUM_CHRONO_TICKS)) * (M_PI / 180.0f);
        bool is_major = (t % 4 == 0); // Major tick every 60°
        float r_inner = is_major ? 162.0f : 166.0f;
        float r_outer = 171.0f;

        tick_pts[t][0].x = 180 + (int16_t)(r_inner * cosf(angle_rad));
        tick_pts[t][0].y = 180 + (int16_t)(r_inner * sinf(angle_rad));
        tick_pts[t][1].x = 180 + (int16_t)(r_outer * cosf(angle_rad));
        tick_pts[t][1].y = 180 + (int16_t)(r_outer * sinf(angle_rad));

        chrono_ticks[t] = lv_line_create(teams_page);
        lv_line_set_points(chrono_ticks[t], tick_pts[t], 2);
        lv_obj_set_style_line_width(chrono_ticks[t], is_major ? 2 : 1, 0);
        lv_obj_set_style_line_color(chrono_ticks[t], is_major ? lv_color_hex(0x475569) : lv_color_hex(0x1E293B), 0);
        lv_obj_clear_flag(chrono_ticks[t], LV_OBJ_FLAG_CLICKABLE);
    }

    // 3. Header Status Capsule Pill (Image 3 & Image 4 style)
    pill_meeting = lv_obj_create(teams_page);
    lv_obj_set_size(pill_meeting, 150, 26);
    lv_obj_align(pill_meeting, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_radius(pill_meeting, 13, 0);
    lv_obj_set_style_bg_color(pill_meeting, lv_color_hex(0x121824), 0);
    lv_obj_set_style_border_width(pill_meeting, 1, 0);
    lv_obj_set_style_border_color(pill_meeting, lv_color_hex(0x232E42), 0);
    lv_obj_clear_flag(pill_meeting, LV_OBJ_FLAG_SCROLLABLE);

    dot_meeting = lv_obj_create(pill_meeting);
    lv_obj_set_size(dot_meeting, 8, 8);
    lv_obj_align(dot_meeting, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(dot_meeting, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot_meeting, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_border_width(dot_meeting, 0, 0);

    lbl_meeting = lv_label_create(pill_meeting);
    lv_label_set_text(lbl_meeting, "STANDBY");
    lv_obj_set_style_text_font(lbl_meeting, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_meeting, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(lbl_meeting, LV_ALIGN_CENTER, 6, 0);

    // 4. Hero Mic Concentric Dial (Centered at X = 0, Y = -20)
    mic_outer_halo = lv_obj_create(teams_page);
    lv_obj_set_size(mic_outer_halo, 130, 130);
    lv_obj_align(mic_outer_halo, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_radius(mic_outer_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(mic_outer_halo, lv_color_hex(0x161C26), 0);
    lv_obj_set_style_border_width(mic_outer_halo, 2, 0);
    lv_obj_set_style_border_color(mic_outer_halo, lv_color_hex(0xEF4444), 0);
    lv_obj_clear_flag(mic_outer_halo, LV_OBJ_FLAG_SCROLLABLE);

    btn_mic = lv_btn_create(teams_page);
    lv_obj_set_size(btn_mic, 114, 114);
    lv_obj_align_to(btn_mic, mic_outer_halo, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(btn_mic, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0x3B1218), 0); // Crimson dark core
    lv_obj_set_style_border_width(btn_mic, 1, 0);
    lv_obj_set_style_border_color(btn_mic, lv_color_hex(0xEF4444), 0);
    lv_obj_add_event_cb(btn_mic, btn_mic_cb, LV_EVENT_CLICKED, NULL);

    lbl_mic_icon = lv_label_create(btn_mic);
    lv_label_set_text(lbl_mic_icon, LV_SYMBOL_MUTE);
    lv_obj_set_style_text_font(lbl_mic_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_mic_icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_mic_icon, LV_ALIGN_CENTER, 0, -12);

    lbl_mic_txt = lv_label_create(btn_mic);
    lv_label_set_text(lbl_mic_txt, "MIC MUTED");
    lv_obj_set_style_text_font(lbl_mic_txt, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_mic_txt, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_mic_txt, LV_ALIGN_CENTER, 0, 16);

    // 5. Raise / Lower Hand Capsule Button (Positioned at Y = 74)
    btn_hand = lv_btn_create(teams_page);
    lv_obj_set_size(btn_hand, 164, 46);
    lv_obj_align(btn_hand, LV_ALIGN_CENTER, 0, 74);
    lv_obj_set_style_radius(btn_hand, 23, 0);
    lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0x131926), 0);
    lv_obj_set_style_border_width(btn_hand, 1, 0);
    lv_obj_set_style_border_color(btn_hand, lv_color_hex(0x28354D), 0);
    lv_obj_add_event_cb(btn_hand, btn_hand_cb, LV_EVENT_CLICKED, NULL);

    lbl_hand = lv_label_create(btn_hand);
    lv_label_set_text(lbl_hand, "RAISE HAND");
    lv_obj_set_style_text_font(lbl_hand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_hand, lv_color_hex(0xA8B4C8), 0);
    lv_obj_center(lbl_hand);

    // 6. Bottom Micro-Caption
    lbl_hint = lv_label_create(teams_page);
    lv_label_set_text(lbl_hint, "TEAMS CONTROLLER");
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x475569), 0);
    lv_obj_align(lbl_hint, LV_ALIGN_CENTER, 0, 114);
}

void page_teams_update(bool in_meeting, bool is_muted, bool hand_raised) {
    teams_in_meeting = in_meeting;
    teams_muted = is_muted;
    teams_hand = hand_raised;

    if (!lbl_meeting || !dot_meeting || !btn_mic || !lbl_mic_txt || !lbl_mic_icon || !btn_hand || !lbl_hand) return;

    // 1. Meeting Status Pill
    if (in_meeting) {
        lv_label_set_text(lbl_meeting, "IN MEETING");
        lv_obj_set_style_text_color(lbl_meeting, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_bg_color(dot_meeting, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_border_color(pill_meeting, lv_color_hex(0x00FF88), 0);
    } else {
        lv_label_set_text(lbl_meeting, "STANDBY");
        lv_obj_set_style_text_color(lbl_meeting, lv_color_hex(0x94A3B8), 0);
        lv_obj_set_style_bg_color(dot_meeting, lv_color_hex(0x64748B), 0);
        lv_obj_set_style_border_color(pill_meeting, lv_color_hex(0x232E42), 0);
    }

    // 2. Mic Button State
    if (is_muted) {
        lv_obj_set_style_border_color(mic_outer_halo, lv_color_hex(0xEF4444), 0);
        lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0x3B1218), 0);
        lv_obj_set_style_border_color(btn_mic, lv_color_hex(0xEF4444), 0);
        lv_label_set_text(lbl_mic_icon, LV_SYMBOL_MUTE);
        lv_label_set_text(lbl_mic_txt, "MIC MUTED");
    } else {
        lv_obj_set_style_border_color(mic_outer_halo, lv_color_hex(0x10B981), 0);
        lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0x063B26), 0);
        lv_obj_set_style_border_color(btn_mic, lv_color_hex(0x10B981), 0);
        lv_label_set_text(lbl_mic_icon, LV_SYMBOL_AUDIO);
        lv_label_set_text(lbl_mic_txt, "MIC LIVE");
    }

    // 3. Hand Button State
    if (hand_raised) {
        lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0x422406), 0);
        lv_obj_set_style_border_color(btn_hand, lv_color_hex(0xF59E0B), 0);
        lv_obj_set_style_text_color(lbl_hand, lv_color_hex(0xFDE68A), 0);
        lv_label_set_text(lbl_hand, "HAND RAISED");
    } else {
        lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0x131926), 0);
        lv_obj_set_style_border_color(btn_hand, lv_color_hex(0x28354D), 0);
        lv_obj_set_style_text_color(lbl_hand, lv_color_hex(0xA8B4C8), 0);
        lv_label_set_text(lbl_hand, "RAISE HAND");
    }

    // Sync LED ring
    led_ring_set_meeting_state(in_meeting, is_muted, hand_raised);
}
