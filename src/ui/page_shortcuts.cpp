#include "page_shortcuts.h"
#include "../config.h"
#include "../hal/led_ring.h"
#include "../hal/audio_buzzer.h"
#include <math.h>

#define NUM_SHORTCUTS 6
#define HOLD_DURATION_MS 2000 // 2-second hold interval

// Distinct vibrant palette matching Picture 1 & Picture 2
static const lv_color_t shortcut_colors[NUM_SHORTCUTS] = {
    LV_COLOR_MAKE(0xFF, 0x57, 0x22), // 1: Vibrant Coral Orange (Picture 1 hero color)
    LV_COLOR_MAKE(0x29, 0x79, 0xFF), // 2: Electric Royal Blue (Picture 2 hero color)
    LV_COLOR_MAKE(0x9C, 0x27, 0xB0), // 3: Deep Violet / Purple
    LV_COLOR_MAKE(0xE9, 0x1E, 0x63), // 4: Vivid Rose Pink
    LV_COLOR_MAKE(0xFF, 0x98, 0x00), // 5: Golden Amber
    LV_COLOR_MAKE(0x00, 0xE6, 0x76)  // 6: Spring Emerald Green
};

// Built-in LVGL symbol icons for each shortcut
static const char *shortcut_icons[NUM_SHORTCUTS] = {
    LV_SYMBOL_IMAGE,      // 1: Camera / Image
    LV_SYMBOL_SETTINGS,   // 2: Dev Tools / Settings
    LV_SYMBOL_WIFI,       // 3: Browser / Network
    LV_SYMBOL_PLUS,       // 4: Calculator / Utilities
    LV_SYMBOL_EDIT,       // 5: Notepad / Editor
    LV_SYMBOL_DIRECTORY   // 6: Explorer / Files
};

static char shortcut_names[NUM_SHORTCUTS][32] = {
    "Terminal",
    "VS Code",
    "Browser",
    "Calculator",
    "Notepad",
    "Explorer"
};

// Angular boundaries for the 6 fixed sectors (each 60° wide)
// Sector 0: Top (240° to 300°, center at 270°)
// Sector 1: Top-Right (300° to 360°, center at 330°)
// Sector 2: Bottom-Right (0° to 60°, center at 30°)
// Sector 3: Bottom (60° to 120°, center at 90°)
// Sector 4: Bottom-Left (120° to 180°, center at 150°)
// Sector 5: Top-Left (180° to 240°, center at 210°)
static const uint16_t sector_start_angles[NUM_SHORTCUTS] = {240, 300, 0,   60,  120, 180};
static const uint16_t sector_end_angles[NUM_SHORTCUTS]   = {300, 360, 60,  120, 180, 240};
static const float    sector_center_angles[NUM_SHORTCUTS]= {270.0f, 330.0f, 30.0f, 90.0f, 150.0f, 210.0f};

static lv_obj_t *shortcuts_page = NULL;
static lv_obj_t *dial_base = NULL;
static lv_obj_t *active_wedge = NULL;
static lv_obj_t *wedge_accent_cap = NULL;
static lv_obj_t *radial_lines[NUM_SHORTCUTS];
static lv_point_t line_pts[NUM_SHORTCUTS][2];

// 6 STATIC sector item buttons (their positions NEVER change)
static lv_obj_t *sector_items[NUM_SHORTCUTS];
static lv_obj_t *sector_icons[NUM_SHORTCUTS];
static lv_obj_t *sector_labels[NUM_SHORTCUTS];

// Concentric center hub
static lv_obj_t *hub_outer_ring = NULL;
static lv_obj_t *hub_progress_arc = NULL;
static lv_obj_t *hub_inner_ring = NULL;
static lv_obj_t *hub_center_core = NULL;
static lv_obj_t *lbl_core_num = NULL;
static lv_obj_t *lbl_core_name = NULL;
static lv_obj_t *lbl_core_status = NULL;

static int active_idx = 0;
static bool is_holding = false;
static bool is_launched = false;
static uint32_t press_start_ms = 0;
static uint32_t launch_time_ms = 0;

static void update_dial_state();

static void trigger_launch() {
    is_launched = true;
    launch_time_ms = millis();

    // Visual feedback
    lv_arc_set_value(hub_progress_arc, 1000);
    lv_label_set_text(lbl_core_status, "LAUNCHED!");
    lv_obj_set_style_text_color(lbl_core_status, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_bg_color(hub_center_core, lv_color_hex(0x153A26), 0);

    // Audio & LED feedback
    audio_buzzer_trigger_alert(200);

    lv_color_t col = shortcut_colors[active_idx];
    led_ring_flash_stopwatch(LV_COLOR_GET_R(col), LV_COLOR_GET_G(col), LV_COLOR_GET_B(col), 500);

    // Send Serial command to companion app
    int shortcut_num = active_idx + 1;
    Serial.printf("{\"cmd\":\"shortcut\",\"num\":%d}\n", shortcut_num);
}

static void on_touch_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        if (!is_launched) {
            is_holding = true;
            press_start_ms = millis();
            lv_label_set_text(lbl_core_status, "HOLD 2s");
            lv_obj_set_style_text_color(lbl_core_status, lv_color_hex(0xFFFFFF), 0);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        is_holding = false;
        if (!is_launched) {
            page_shortcuts_reset();
        }
    }
}

static void on_sector_click(lv_event_t *e) {
    int target_idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (target_idx >= 0 && target_idx < NUM_SHORTCUTS) {
        active_idx = target_idx;
        update_dial_state();
        page_shortcuts_reset();
    }
}

void page_shortcuts_create(lv_obj_t *parent) {
    shortcuts_page = parent;
    lv_obj_set_style_bg_color(shortcuts_page, lv_color_hex(0x101217), 0);

    // 1. Dark Base Wheel (Diameter 340px)
    dial_base = lv_obj_create(shortcuts_page);
    lv_obj_set_size(dial_base, 340, 340);
    lv_obj_center(dial_base);
    lv_obj_set_style_radius(dial_base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dial_base, lv_color_hex(0x1B1E26), 0);
    lv_obj_set_style_border_width(dial_base, 3, 0);
    lv_obj_set_style_border_color(dial_base, lv_color_hex(0x282D38), 0);
    lv_obj_clear_flag(dial_base, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Radial Divider Lines (Fixed at 240°, 300°, 0°, 60°, 120°, 180°)
    static const float divider_angles[NUM_SHORTCUTS] = {240.0f, 300.0f, 0.0f, 60.0f, 120.0f, 180.0f};
    for (int i = 0; i < NUM_SHORTCUTS; i++) {
        float rad = divider_angles[i] * (M_PI / 180.0f);
        line_pts[i][0].x = 180 + (int16_t)(62.0f * cosf(rad));
        line_pts[i][0].y = 180 + (int16_t)(62.0f * sinf(rad));
        line_pts[i][1].x = 180 + (int16_t)(168.0f * cosf(rad));
        line_pts[i][1].y = 180 + (int16_t)(168.0f * sinf(rad));

        radial_lines[i] = lv_line_create(shortcuts_page);
        lv_line_set_points(radial_lines[i], line_pts[i], 2);
        lv_obj_set_style_line_width(radial_lines[i], 1, 0);
        lv_obj_set_style_line_color(radial_lines[i], lv_color_hex(0x282E3A), 0);
        lv_obj_clear_flag(radial_lines[i], LV_OBJ_FLAG_CLICKABLE);
    }

    // 3. Rotating Selection Wedge (Moves to active shortcut sector on knob turn)
    active_wedge = lv_arc_create(shortcuts_page);
    lv_obj_set_size(active_wedge, 338, 338);
    lv_obj_center(active_wedge);
    lv_arc_set_angles(active_wedge, sector_start_angles[0], sector_end_angles[0]);
    lv_arc_set_bg_angles(active_wedge, sector_start_angles[0], sector_end_angles[0]);
    lv_obj_set_style_arc_width(active_wedge, 108, LV_PART_MAIN);
    lv_obj_set_style_arc_color(active_wedge, shortcut_colors[0], LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(active_wedge, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(active_wedge, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(active_wedge, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(active_wedge, LV_OBJ_FLAG_CLICKABLE);

    // Rotating outer perimeter accent cap (Picture 2 style)
    wedge_accent_cap = lv_arc_create(shortcuts_page);
    lv_obj_set_size(wedge_accent_cap, 342, 342);
    lv_obj_center(wedge_accent_cap);
    lv_arc_set_angles(wedge_accent_cap, sector_start_angles[0] + 1, sector_end_angles[0] - 1);
    lv_arc_set_bg_angles(wedge_accent_cap, sector_start_angles[0] + 1, sector_end_angles[0] - 1);
    lv_obj_set_style_arc_width(wedge_accent_cap, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(wedge_accent_cap, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(wedge_accent_cap, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(wedge_accent_cap, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(wedge_accent_cap, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(wedge_accent_cap, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(wedge_accent_cap, LV_OBJ_FLAG_CLICKABLE);

    // 4. 6 STATIC Sector Items (Shortcuts 1 to 6 permanently fixed at their angles)
    float R = 115.0f; // Center radius of sectors
    for (int i = 0; i < NUM_SHORTCUTS; i++) {
        float rad = sector_center_angles[i] * (M_PI / 180.0f);
        int cx = 180 + (int)(R * cosf(rad));
        int cy = 180 + (int)(R * sinf(rad));

        sector_items[i] = lv_btn_create(shortcuts_page);
        lv_obj_set_size(sector_items[i], 70, 60);
        lv_obj_align(sector_items[i], LV_ALIGN_CENTER, cx - 180, cy - 180);
        lv_obj_set_style_radius(sector_items[i], 12, 0);
        lv_obj_set_style_bg_opa(sector_items[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sector_items[i], 0, 0);
        lv_obj_clear_flag(sector_items[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(sector_items[i], on_sector_click, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        sector_icons[i] = lv_label_create(sector_items[i]);
        lv_label_set_text(sector_icons[i], shortcut_icons[i]);
        lv_obj_set_style_text_font(sector_icons[i], &lv_font_montserrat_20, 0);
        lv_obj_align(sector_icons[i], LV_ALIGN_CENTER, 0, -10);

        sector_labels[i] = lv_label_create(sector_items[i]);
        lv_label_set_text(sector_labels[i], shortcut_names[i]);
        lv_obj_set_style_text_font(sector_labels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_width(sector_labels[i], 68);
        lv_label_set_long_mode(sector_labels[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_align(sector_labels[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(sector_labels[i], LV_ALIGN_CENTER, 0, 12);
    }

    // 5. Concentric Center Hub (Picture 1 & Picture 2)
    // Outer border ring
    hub_outer_ring = lv_obj_create(shortcuts_page);
    lv_obj_set_size(hub_outer_ring, 126, 126);
    lv_obj_center(hub_outer_ring);
    lv_obj_set_style_radius(hub_outer_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_outer_ring, lv_color_hex(0x13161C), 0);
    lv_obj_set_style_border_width(hub_outer_ring, 2, 0);
    lv_obj_set_style_border_color(hub_outer_ring, shortcut_colors[0], 0);
    lv_obj_clear_flag(hub_outer_ring, LV_OBJ_FLAG_SCROLLABLE);

    // Circular 2-Second Hold Progress Arc (Diameter 126px)
    hub_progress_arc = lv_arc_create(shortcuts_page);
    lv_obj_set_size(hub_progress_arc, 126, 126);
    lv_obj_center(hub_progress_arc);
    lv_arc_set_rotation(hub_progress_arc, 270); // Sweeps clockwise from top
    lv_arc_set_bg_angles(hub_progress_arc, 0, 360);
    lv_arc_set_angles(hub_progress_arc, 0, 0);
    lv_arc_set_range(hub_progress_arc, 0, 1000);
    lv_arc_set_value(hub_progress_arc, 0);
    lv_obj_set_style_arc_width(hub_progress_arc, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_color(hub_progress_arc, lv_color_hex(0x222836), LV_PART_MAIN);
    lv_obj_set_style_arc_width(hub_progress_arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(hub_progress_arc, shortcut_colors[0], LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(hub_progress_arc, true, LV_PART_INDICATOR);
    lv_obj_remove_style(hub_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(hub_progress_arc, LV_OBJ_FLAG_CLICKABLE);

    // Intermediate concentric silver/metallic accent ring
    hub_inner_ring = lv_obj_create(shortcuts_page);
    lv_obj_set_size(hub_inner_ring, 108, 108);
    lv_obj_center(hub_inner_ring);
    lv_obj_set_style_radius(hub_inner_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_inner_ring, lv_color_hex(0xD1D5DB), 0);
    lv_obj_set_style_border_width(hub_inner_ring, 0, 0);
    lv_obj_clear_flag(hub_inner_ring, LV_OBJ_FLAG_SCROLLABLE);

    // Center Core Button (Touch target for long press)
    hub_center_core = lv_btn_create(shortcuts_page);
    lv_obj_set_size(hub_center_core, 88, 88);
    lv_obj_center(hub_center_core);
    lv_obj_set_style_radius(hub_center_core, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_center_core, shortcut_colors[0], 0);
    lv_obj_set_style_border_width(hub_center_core, 0, 0);
    lv_obj_clear_flag(hub_center_core, LV_OBJ_FLAG_SCROLLABLE);

    // Enable touch hold on center core and the whole page
    lv_obj_add_event_cb(hub_center_core, on_touch_event, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(shortcuts_page, on_touch_event, LV_EVENT_ALL, NULL);

    // Core Content
    lbl_core_num = lv_label_create(hub_center_core);
    lv_label_set_text(lbl_core_num, "#1");
    lv_obj_set_style_text_font(lbl_core_num, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_core_num, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_core_num, LV_ALIGN_CENTER, 0, -18);

    lbl_core_name = lv_label_create(hub_center_core);
    lv_label_set_text(lbl_core_name, shortcut_names[0]);
    lv_obj_set_style_text_font(lbl_core_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_core_name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(lbl_core_name, 80);
    lv_label_set_long_mode(lbl_core_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(lbl_core_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_core_name, LV_ALIGN_CENTER, 0, 0);

    lbl_core_status = lv_label_create(hub_center_core);
    lv_label_set_text(lbl_core_status, "HOLD 2s");
    lv_obj_set_style_text_font(lbl_core_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_core_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_core_status, LV_ALIGN_CENTER, 0, 18);

    update_dial_state();
}

static void update_dial_state() {
    lv_color_t active_col = shortcut_colors[active_idx];
    uint16_t start_ang = sector_start_angles[active_idx];
    uint16_t end_ang = sector_end_angles[active_idx];

    // 1. ROTATE SELECTION WEDGE to active shortcut sector!
    lv_arc_set_angles(active_wedge, start_ang, end_ang);
    lv_arc_set_bg_angles(active_wedge, start_ang, end_ang);
    lv_obj_set_style_arc_color(active_wedge, active_col, LV_PART_MAIN);

    // Rotate outer accent cap
    uint16_t cap_start = (start_ang == 0) ? 1 : start_ang + 1;
    uint16_t cap_end = (end_ang == 360) ? 359 : end_ang - 1;
    lv_arc_set_angles(wedge_accent_cap, cap_start, cap_end);
    lv_arc_set_bg_angles(wedge_accent_cap, cap_start, cap_end);

    // 2. Update all 6 STATIC shortcuts (their positions remain fixed!)
    for (int i = 0; i < NUM_SHORTCUTS; i++) {
        if (i == active_idx) {
            // Selected shortcut: highlight with pure white text & prominent font
            lv_obj_set_style_text_color(sector_icons[i], lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_color(sector_labels[i], lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(sector_icons[i], &lv_font_montserrat_20, 0);
        } else {
            // Inactive shortcuts: muted slate styling on dark base
            lv_obj_set_style_text_color(sector_icons[i], lv_color_hex(0x8F9BAE), 0);
            lv_obj_set_style_text_color(sector_labels[i], lv_color_hex(0x5A6678), 0);
            lv_obj_set_style_text_font(sector_icons[i], &lv_font_montserrat_16, 0);
        }
        lv_label_set_text(sector_labels[i], shortcut_names[i]);
    }

    // 3. Update Center Hub
    lv_obj_set_style_border_color(hub_outer_ring, active_col, 0);
    lv_obj_set_style_arc_color(hub_progress_arc, active_col, LV_PART_INDICATOR);
    lv_arc_set_value(hub_progress_arc, 0);

    lv_obj_set_style_bg_color(hub_center_core, active_col, 0);

    char num_buf[8];
    snprintf(num_buf, sizeof(num_buf), "#%d", active_idx + 1);
    lv_label_set_text(lbl_core_num, num_buf);
    lv_label_set_text(lbl_core_name, shortcut_names[active_idx]);

    lv_label_set_text(lbl_core_status, "HOLD 2s");
    lv_obj_set_style_text_color(lbl_core_status, lv_color_hex(0xFFFFFF), 0);

    // 4. LED Ring feedback
    led_ring_flash_stopwatch(LV_COLOR_GET_R(active_col), LV_COLOR_GET_G(active_col), LV_COLOR_GET_B(active_col), 150);
}

void page_shortcuts_on_knob(int delta) {
    if (delta == 0) return;

    if (delta > 0) {
        active_idx = (active_idx + delta) % NUM_SHORTCUTS;
    } else {
        active_idx = (active_idx + delta) % NUM_SHORTCUTS;
        if (active_idx < 0) active_idx += NUM_SHORTCUTS;
    }

    page_shortcuts_reset();
    update_dial_state();
}

void page_shortcuts_update() {
    if (!shortcuts_page) return;

    // Reset back to idle state after launch pulse
    if (is_launched) {
        if (millis() - launch_time_ms > 1500) {
            is_launched = false;
            page_shortcuts_reset();
        }
        return;
    }

    // Continuous 2-second hold charging
    if (is_holding) {
        uint32_t elapsed = millis() - press_start_ms;

        if (elapsed >= HOLD_DURATION_MS) {
            trigger_launch();
        } else {
            int permille = (int)((elapsed * 1000ULL) / HOLD_DURATION_MS);
            if (permille > 1000) permille = 1000;
            lv_arc_set_value(hub_progress_arc, permille);

            float remain_s = (float)(HOLD_DURATION_MS - elapsed) / 1000.0f;
            if (remain_s < 0.0f) remain_s = 0.0f;
            char tbuf[16];
            snprintf(tbuf, sizeof(tbuf), "%.1fs...", remain_s);
            lv_label_set_text(lbl_core_status, tbuf);
        }
    }
}

void page_shortcuts_reset() {
    is_holding = false;
    is_launched = false;
    if (hub_progress_arc) {
        lv_arc_set_value(hub_progress_arc, 0);
    }
    if (lbl_core_status) {
        lv_label_set_text(lbl_core_status, "HOLD 2s");
        lv_obj_set_style_text_color(lbl_core_status, lv_color_hex(0xFFFFFF), 0);
    }
    if (hub_center_core && active_idx >= 0 && active_idx < NUM_SHORTCUTS) {
        lv_obj_set_style_bg_color(hub_center_core, shortcut_colors[active_idx], 0);
    }
}

void page_shortcuts_set_name(int index, const char *name) {
    if (index >= 0 && index < NUM_SHORTCUTS && name && strlen(name) > 0) {
        strncpy(shortcut_names[index], name, sizeof(shortcut_names[index]) - 1);
        shortcut_names[index][sizeof(shortcut_names[index]) - 1] = '\0';
        if (sector_labels[index]) {
            lv_label_set_text(sector_labels[index], shortcut_names[index]);
        }
        if (index == active_idx && lbl_core_name) {
            lv_label_set_text(lbl_core_name, shortcut_names[index]);
        }
    }
}

int page_shortcuts_get_active() {
    return active_idx + 1;
}
