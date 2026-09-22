#include "page_teams.h"
#include "../hal/led_ring.h"

static lv_obj_t *teams_page = NULL;
static lv_obj_t *lbl_status = NULL;
static lv_obj_t *btn_mic = NULL;
static lv_obj_t *lbl_mic = NULL;
static lv_obj_t *btn_hand = NULL;
static lv_obj_t *lbl_hand = NULL;

static bool teams_in_meeting = false;
static bool teams_muted = true;
static bool teams_hand = false;

static void btn_mic_cb(lv_event_t *e) {
    teams_muted = !teams_muted;
    // Tell serial protocol to toggle Teams mute
    Serial.println("{\"cmd\":\"teams_toggle_mute\"}");
    page_teams_update(teams_in_meeting, teams_muted, teams_hand);
}

static void btn_hand_cb(lv_event_t *e) {
    teams_hand = !teams_hand;
    // Tell serial protocol to toggle Teams hand
    Serial.println("{\"cmd\":\"teams_toggle_hand\"}");
    page_teams_update(teams_in_meeting, teams_muted, teams_hand);
}

void page_teams_create(lv_obj_t *parent) {
    teams_page = parent;
    lv_obj_set_style_bg_color(teams_page, lv_color_hex(0x0F111A), 0);

    // Header Status
    lbl_status = lv_label_create(teams_page);
    lv_label_set_text(lbl_status, "MS TEAMS - READY");
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x8A9FB4), 0);
    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 35);

    // Big Mic Mute / Unmute Button
    btn_mic = lv_btn_create(teams_page);
    lv_obj_set_size(btn_mic, 160, 75);
    lv_obj_align(btn_mic, LV_ALIGN_CENTER, 0, -25);
    lv_obj_set_style_radius(btn_mic, 20, 0);
    lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0xE63946), 0); // Default Muted Red
    lv_obj_add_event_cb(btn_mic, btn_mic_cb, LV_EVENT_CLICKED, NULL);

    lbl_mic = lv_label_create(btn_mic);
    lv_label_set_text(lbl_mic, "MIC MUTED");
    lv_obj_set_style_text_font(lbl_mic, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_mic, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_mic);

    // Raise / Lower Hand Button
    btn_hand = lv_btn_create(teams_page);
    lv_obj_set_size(btn_hand, 160, 55);
    lv_obj_align(btn_hand, LV_ALIGN_CENTER, 0, 55);
    lv_obj_set_style_radius(btn_hand, 18, 0);
    lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0x242D3D), 0); // Default inactive
    lv_obj_add_event_cb(btn_hand, btn_hand_cb, LV_EVENT_CLICKED, NULL);

    lbl_hand = lv_label_create(btn_hand);
    lv_label_set_text(lbl_hand, "RAISE HAND");
    lv_obj_set_style_text_font(lbl_hand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_hand, lv_color_hex(0xDFE6ED), 0);
    lv_obj_center(lbl_hand);
}

void page_teams_update(bool in_meeting, bool is_muted, bool hand_raised) {
    teams_in_meeting = in_meeting;
    teams_muted = is_muted;
    teams_hand = hand_raised;

    if (!lbl_status || !btn_mic || !lbl_mic || !btn_hand || !lbl_hand) return;

    if (in_meeting) {
        lv_label_set_text(lbl_status, "IN MEETING");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF88), 0);
    } else {
        lv_label_set_text(lbl_status, "NOT IN MEETING");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x8A9FB4), 0);
    }

    if (is_muted) {
        lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0xE63946), 0); // Red
        lv_label_set_text(lbl_mic, "MIC MUTED");
    } else {
        lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0x2A9D8F), 0); // Green
        lv_label_set_text(lbl_mic, "MIC LIVE");
    }

    if (hand_raised) {
        lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0xF4A261), 0); // Yellow/Orange
        lv_label_set_text(lbl_hand, "HAND RAISED");
    } else {
        lv_obj_set_style_bg_color(btn_hand, lv_color_hex(0x242D3D), 0);
        lv_label_set_text(lbl_hand, "RAISE HAND");
    }

    // Sync LED ring
    led_ring_set_meeting_state(in_meeting, is_muted, hand_raised);
}
