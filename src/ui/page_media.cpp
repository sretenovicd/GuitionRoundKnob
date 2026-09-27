#include "page_media.h"
#include "../hal/usb_manager.h"
#include <math.h>

#define NUM_CHRONO_TICKS 24

static lv_obj_t *media_page = NULL;
static lv_obj_t *dial_base = NULL;
static lv_obj_t *chrono_ticks[NUM_CHRONO_TICKS];
static lv_point_t tick_pts[NUM_CHRONO_TICKS][2];

// Vinyl concentric grooved disc (Image 2 style)
static lv_obj_t *art_badge = NULL;
static lv_obj_t *art_ring1 = NULL;
static lv_obj_t *art_ring2 = NULL;
static lv_obj_t *art_dot = NULL;

static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_artist = NULL;
static lv_obj_t *bar_progress = NULL;
static lv_obj_t *lbl_time = NULL;

static lv_obj_t *btn_prev = NULL;
static lv_obj_t *btn_play = NULL;
static lv_obj_t *lbl_play = NULL;
static lv_obj_t *btn_next = NULL;

static lv_color_t accent_col;

static void btn_prev_cb(lv_event_t *e) {
    usb_send_prev_track();
}

static void btn_play_cb(lv_event_t *e) {
    usb_send_play_pause();
}

static void btn_next_cb(lv_event_t *e) {
    usb_send_next_track();
}

void page_media_create(lv_obj_t *parent) {
    media_page = parent;
    accent_col = lv_color_hex(0x00D2FF); // Default Electric Cyan

    lv_obj_set_style_bg_color(media_page, lv_color_hex(0x06080D), 0); // Pure Obsidian Canvas

    // 1. Dial Base Disc (Diameter 344px)
    dial_base = lv_obj_create(media_page);
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
        bool is_major = (t % 4 == 0);
        float r_inner = is_major ? 162.0f : 166.0f;
        float r_outer = 171.0f;

        tick_pts[t][0].x = 180 + (int16_t)(r_inner * cosf(angle_rad));
        tick_pts[t][0].y = 180 + (int16_t)(r_inner * sinf(angle_rad));
        tick_pts[t][1].x = 180 + (int16_t)(r_outer * cosf(angle_rad));
        tick_pts[t][1].y = 180 + (int16_t)(r_outer * sinf(angle_rad));

        chrono_ticks[t] = lv_line_create(media_page);
        lv_line_set_points(chrono_ticks[t], tick_pts[t], 2);
        lv_obj_set_style_line_width(chrono_ticks[t], is_major ? 2 : 1, 0);
        lv_obj_set_style_line_color(chrono_ticks[t], is_major ? lv_color_hex(0x475569) : lv_color_hex(0x1E293B), 0);
        lv_obj_clear_flag(chrono_ticks[t], LV_OBJ_FLAG_CLICKABLE);
    }

    // 3. Concentric Vinyl Grooved Disc (Top center, Image 2 style)
    art_badge = lv_obj_create(media_page);
    lv_obj_set_size(art_badge, 56, 56);
    lv_obj_align(art_badge, LV_ALIGN_CENTER, 0, -112);
    lv_obj_set_style_radius(art_badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(art_badge, lv_color_hex(0x131824), 0);
    lv_obj_set_style_border_width(art_badge, 2, 0);
    lv_obj_set_style_border_color(art_badge, accent_col, 0);
    lv_obj_clear_flag(art_badge, LV_OBJ_FLAG_SCROLLABLE);

    art_ring1 = lv_obj_create(art_badge);
    lv_obj_set_size(art_ring1, 40, 40);
    lv_obj_center(art_ring1);
    lv_obj_set_style_radius(art_ring1, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(art_ring1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(art_ring1, 1, 0);
    lv_obj_set_style_border_color(art_ring1, lv_color_hex(0x28354A), 0);
    lv_obj_clear_flag(art_ring1, LV_OBJ_FLAG_SCROLLABLE);

    art_ring2 = lv_obj_create(art_badge);
    lv_obj_set_size(art_ring2, 26, 26);
    lv_obj_center(art_ring2);
    lv_obj_set_style_radius(art_ring2, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(art_ring2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(art_ring2, 1, 0);
    lv_obj_set_style_border_color(art_ring2, lv_color_hex(0x28354A), 0);
    lv_obj_clear_flag(art_ring2, LV_OBJ_FLAG_SCROLLABLE);

    art_dot = lv_obj_create(art_badge);
    lv_obj_set_size(art_dot, 14, 14);
    lv_obj_center(art_dot);
    lv_obj_set_style_radius(art_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(art_dot, accent_col, 0);
    lv_obj_set_style_border_width(art_dot, 0, 0);

    // 4. Hero Track Title (Clean High-Contrast Typography)
    lbl_title = lv_label_create(media_page);
    lv_label_set_text(lbl_title, "No Media Playing");
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_title, 260);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, -58);

    // 5. Artist Name (Atmospheric Horizon Glow Accent)
    lbl_artist = lv_label_create(media_page);
    lv_label_set_text(lbl_artist, "Tidal / YouTube / Spotify");
    lv_label_set_long_mode(lbl_artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_artist, 240);
    lv_obj_set_style_text_align(lbl_artist, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_artist, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_artist, accent_col, 0);
    lv_obj_align(lbl_artist, LV_ALIGN_CENTER, 0, -32);

    // 6. Precision Progress Track
    bar_progress = lv_bar_create(media_page);
    lv_obj_set_size(bar_progress, 240, 6);
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, -2);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, lv_color_hex(0x1B2332), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, accent_col, LV_PART_INDICATOR);

    // 7. Time Elapsed / Total Label
    lbl_time = lv_label_create(media_page);
    lv_label_set_text(lbl_time, "--:-- / --:--");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x7E8B9F), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, 16);

    // 8. Control Buttons Container (Circular Controls, Image 2 style)
    lv_obj_t *ctrl_box = lv_obj_create(media_page);
    lv_obj_set_size(ctrl_box, 260, 72);
    lv_obj_align(ctrl_box, LV_ALIGN_CENTER, 0, 72);
    lv_obj_set_style_bg_opa(ctrl_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl_box, 0, 0);
    lv_obj_set_flex_flow(ctrl_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl_box, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Previous Button
    btn_prev = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_prev, 52, 52);
    lv_obj_set_style_radius(btn_prev, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x141A26), 0);
    lv_obj_set_style_border_width(btn_prev, 1, 0);
    lv_obj_set_style_border_color(btn_prev, lv_color_hex(0x263348), 0);
    lv_obj_add_event_cb(btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_prev = lv_label_create(btn_prev);
    lv_label_set_text(lbl_prev, "<<");
    lv_obj_set_style_text_font(lbl_prev, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_prev, lv_color_hex(0xE2E8F0), 0);
    lv_obj_center(lbl_prev);

    // Play/Pause Button (Hero Center Button)
    btn_play = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_play, 64, 64);
    lv_obj_set_style_radius(btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_play, accent_col, 0);
    lv_obj_set_style_border_width(btn_play, 0, 0);
    lv_obj_add_event_cb(btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);

    lbl_play = lv_label_create(btn_play);
    lv_label_set_text(lbl_play, ">");
    lv_obj_set_style_text_color(lbl_play, lv_color_hex(0x06080D), 0);
    lv_obj_set_style_text_font(lbl_play, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_play);

    // Next Button
    btn_next = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_next, 52, 52);
    lv_obj_set_style_radius(btn_next, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x141A26), 0);
    lv_obj_set_style_border_width(btn_next, 1, 0);
    lv_obj_set_style_border_color(btn_next, lv_color_hex(0x263348), 0);
    lv_obj_add_event_cb(btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_next = lv_label_create(btn_next);
    lv_label_set_text(lbl_next, ">>");
    lv_obj_set_style_text_font(lbl_next, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_next, lv_color_hex(0xE2E8F0), 0);
    lv_obj_center(lbl_next);
}

void page_media_set_accent_color(uint8_t r, uint8_t g, uint8_t b) {
    accent_col = lv_color_make(r, g, b);
    if (bar_progress) {
        lv_obj_set_style_bg_color(bar_progress, accent_col, LV_PART_INDICATOR);
    }
    if (btn_play) {
        lv_obj_set_style_bg_color(btn_play, accent_col, 0);
    }
    if (lbl_artist) {
        lv_obj_set_style_text_color(lbl_artist, accent_col, 0);
    }
    if (art_badge && art_dot) {
        lv_obj_set_style_border_color(art_badge, accent_col, 0);
        lv_obj_set_style_bg_color(art_dot, accent_col, 0);
    }
}

void page_media_update(const char *title, const char *artist, bool is_playing, uint32_t pos_ms, uint32_t duration_ms) {
    if (!lbl_title || !lbl_artist || !bar_progress) return;

    if (title && strlen(title) > 0) {
        lv_label_set_text(lbl_title, title);
    } else {
        lv_label_set_text(lbl_title, "No Media Playing");
    }

    if (artist && strlen(artist) > 0) {
        lv_label_set_text(lbl_artist, artist);
    } else {
        lv_label_set_text(lbl_artist, "Tidal / YouTube / Spotify");
    }

    if (lbl_play) {
        lv_label_set_text(lbl_play, is_playing ? "||" : ">");
    }

    if (duration_ms > 0) {
        int pct = (int)((pos_ms * 100ULL) / duration_ms);
        if (pct > 100) pct = 100;
        lv_bar_set_value(bar_progress, pct, LV_ANIM_OFF);

        if (lbl_time) {
            uint32_t pos_s = pos_ms / 1000;
            uint32_t dur_s = duration_ms / 1000;
            char tbuf[32];
            snprintf(tbuf, sizeof(tbuf), "%02lu:%02lu / %02lu:%02lu",
                     (unsigned long)(pos_s / 60), (unsigned long)(pos_s % 60),
                     (unsigned long)(dur_s / 60), (unsigned long)(dur_s % 60));
            lv_label_set_text(lbl_time, tbuf);
        }
    } else {
        lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
        if (lbl_time) {
            lv_label_set_text(lbl_time, "--:-- / --:--");
        }
    }
}
