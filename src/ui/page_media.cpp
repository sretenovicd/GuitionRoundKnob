#include "page_media.h"
#include "../hal/usb_manager.h"

static lv_obj_t *media_page = NULL;
static lv_obj_t *art_badge = NULL;
static lv_obj_t *art_dot = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_artist = NULL;
static lv_obj_t *bar_progress = NULL;
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *btn_play = NULL;
static lv_obj_t *lbl_play = NULL;

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
    lv_obj_set_style_bg_color(media_page, lv_color_hex(0x0C101B), 0);

    // Vinyl Disc / Album Badge (Top)
    art_badge = lv_obj_create(media_page);
    lv_obj_set_size(art_badge, 52, 52);
    lv_obj_align(art_badge, LV_ALIGN_CENTER, 0, -112);
    lv_obj_set_style_radius(art_badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(art_badge, lv_color_hex(0x151D2A), 0);
    lv_obj_set_style_border_width(art_badge, 2, 0);
    lv_obj_set_style_border_color(art_badge, lv_color_hex(0x00D2FF), 0);
    lv_obj_clear_flag(art_badge, LV_OBJ_FLAG_SCROLLABLE);

    // Center vinyl spindle dot
    art_dot = lv_obj_create(art_badge);
    lv_obj_set_size(art_dot, 16, 16);
    lv_obj_center(art_dot);
    lv_obj_set_style_radius(art_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(art_dot, lv_color_hex(0x00D2FF), 0);
    lv_obj_set_style_border_width(art_dot, 0, 0);

    // Track Title
    lbl_title = lv_label_create(media_page);
    lv_label_set_text(lbl_title, "No Media Playing");
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_title, 260);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, -60);

    // Artist
    lbl_artist = lv_label_create(media_page);
    lv_label_set_text(lbl_artist, "Tidal / YouTube / Spotify");
    lv_label_set_long_mode(lbl_artist, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_artist, 240);
    lv_obj_set_style_text_align(lbl_artist, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_artist, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_artist, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(lbl_artist, LV_ALIGN_CENTER, 0, -32);

    // Progress Bar
    bar_progress = lv_bar_create(media_page);
    lv_obj_set_size(bar_progress, 240, 7);
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, lv_color_hex(0x223046), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, lv_color_hex(0x00D2FF), LV_PART_INDICATOR);

    // Time Elapsed / Total Label
    lbl_time = lv_label_create(media_page);
    lv_label_set_text(lbl_time, "--:-- / --:--");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x7C8BA1), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, 18);

    // Control Buttons Container
    lv_obj_t *ctrl_box = lv_obj_create(media_page);
    lv_obj_set_size(ctrl_box, 260, 70);
    lv_obj_align(ctrl_box, LV_ALIGN_CENTER, 0, 75);
    lv_obj_set_style_bg_opa(ctrl_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl_box, 0, 0);
    lv_obj_set_flex_flow(ctrl_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl_box, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Previous Button
    lv_obj_t *btn_prev = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_prev, 55, 55);
    lv_obj_set_style_radius(btn_prev, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x1B2433), 0);
    lv_obj_add_event_cb(btn_prev, btn_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_prev = lv_label_create(btn_prev);
    lv_label_set_text(lbl_prev, "<<");
    lv_obj_center(lbl_prev);

    // Play/Pause Button
    btn_play = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_play, 65, 65);
    lv_obj_set_style_radius(btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0x00D2FF), 0);
    lv_obj_add_event_cb(btn_play, btn_play_cb, LV_EVENT_CLICKED, NULL);
    lbl_play = lv_label_create(btn_play);
    lv_label_set_text(lbl_play, ">");
    lv_obj_set_style_text_color(lbl_play, lv_color_hex(0x0A0E17), 0);
    lv_obj_set_style_text_font(lbl_play, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_play);

    // Next Button
    lv_obj_t *btn_next = lv_btn_create(ctrl_box);
    lv_obj_set_size(btn_next, 55, 55);
    lv_obj_set_style_radius(btn_next, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x1B2433), 0);
    lv_obj_add_event_cb(btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_next = lv_label_create(btn_next);
    lv_label_set_text(lbl_next, ">>");
    lv_obj_center(lbl_next);
}

void page_media_set_accent_color(uint8_t r, uint8_t g, uint8_t b) {
    lv_color_t col = lv_color_make(r, g, b);
    if (bar_progress) {
        lv_obj_set_style_bg_color(bar_progress, col, LV_PART_INDICATOR);
    }
    if (btn_play) {
        lv_obj_set_style_bg_color(btn_play, col, 0);
    }
    if (lbl_artist) {
        lv_obj_set_style_text_color(lbl_artist, col, 0);
    }
    if (art_badge && art_dot) {
        lv_obj_set_style_border_color(art_badge, col, 0);
        lv_obj_set_style_bg_color(art_dot, col, 0);
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

