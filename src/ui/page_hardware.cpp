#include "page_hardware.h"
#include <math.h>

#define NUM_CHRONO_TICKS 24

static lv_obj_t *hw_page = NULL;
static lv_obj_t *dial_base = NULL;
static lv_obj_t *chrono_ticks[NUM_CHRONO_TICKS];
static lv_point_t tick_pts[NUM_CHRONO_TICKS][2];

// CPU Gauge (Obsidian Chrono Concentric)
static lv_obj_t *arc_cpu = NULL;
static lv_obj_t *core_cpu = NULL;
static lv_obj_t *lbl_cpu_val = NULL;
static lv_obj_t *lbl_cpu_tag = NULL;

// RAM Gauge (Obsidian Chrono Concentric)
static lv_obj_t *arc_ram = NULL;
static lv_obj_t *core_ram = NULL;
static lv_obj_t *lbl_ram_val = NULL;
static lv_obj_t *lbl_ram_tag = NULL;

// Network Status Capsules (Image 3 & Image 4 style)
static lv_obj_t *cont_net = NULL;
static lv_obj_t *pill_net_down = NULL;
static lv_obj_t *lbl_net_down = NULL;
static lv_obj_t *pill_net_up = NULL;
static lv_obj_t *lbl_net_up = NULL;

// Backlight Status Capsule (Smart Dial style)
static lv_obj_t *pill_bri = NULL;
static lv_obj_t *lbl_bri = NULL;

void page_hardware_create(lv_obj_t *parent) {
    hw_page = parent;
    lv_obj_set_style_bg_color(hw_page, lv_color_hex(0x06080D), 0); // Pure Obsidian Canvas

    // 1. Dial Base Disc (Diameter 344px)
    dial_base = lv_obj_create(hw_page);
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

        chrono_ticks[t] = lv_line_create(hw_page);
        lv_line_set_points(chrono_ticks[t], tick_pts[t], 2);
        lv_obj_set_style_line_width(chrono_ticks[t], is_major ? 2 : 1, 0);
        lv_obj_set_style_line_color(chrono_ticks[t], is_major ? lv_color_hex(0x475569) : lv_color_hex(0x1E293B), 0);
        lv_obj_clear_flag(chrono_ticks[t], LV_OBJ_FLAG_CLICKABLE);
    }

    // 3. Header Title
    lv_obj_t *lbl_hdr = lv_label_create(hw_page);
    lv_label_set_text(lbl_hdr, "SYSTEM TELEMETRY");
    lv_obj_set_style_text_font(lbl_hdr, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0x64748B), 0);
    lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 22);

    // 4. CPU Concentric Gauge (Left Side, centered at X = -65, Y = -22)
    arc_cpu = lv_arc_create(hw_page);
    lv_obj_set_size(arc_cpu, 122, 122);
    lv_obj_align(arc_cpu, LV_ALIGN_CENTER, -66, -22);
    lv_arc_set_rotation(arc_cpu, 135);
    lv_arc_set_bg_angles(arc_cpu, 0, 270);
    lv_arc_set_range(arc_cpu, 0, 100);
    lv_arc_set_value(arc_cpu, 25);
    lv_obj_set_style_arc_width(arc_cpu, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x161E2C), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_cpu, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x00FF88), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_cpu, true, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_cpu, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_cpu, LV_OBJ_FLAG_CLICKABLE);

    core_cpu = lv_obj_create(hw_page);
    lv_obj_set_size(core_cpu, 96, 96);
    lv_obj_align_to(core_cpu, arc_cpu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(core_cpu, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(core_cpu, lv_color_hex(0x0C1018), 0);
    lv_obj_set_style_border_width(core_cpu, 1, 0);
    lv_obj_set_style_border_color(core_cpu, lv_color_hex(0x1E293B), 0);
    lv_obj_clear_flag(core_cpu, LV_OBJ_FLAG_SCROLLABLE);

    lbl_cpu_val = lv_label_create(core_cpu);
    lv_label_set_text(lbl_cpu_val, "25%");
    lv_obj_set_style_text_font(lbl_cpu_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_cpu_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_cpu_val, LV_ALIGN_CENTER, 0, -8);

    lbl_cpu_tag = lv_label_create(core_cpu);
    lv_label_set_text(lbl_cpu_tag, "CPU");
    lv_obj_set_style_text_font(lbl_cpu_tag, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_cpu_tag, lv_color_hex(0x00FF88), 0);
    lv_obj_align(lbl_cpu_tag, LV_ALIGN_CENTER, 0, 14);

    // 5. RAM Concentric Gauge (Right Side, centered at X = +65, Y = -22)
    arc_ram = lv_arc_create(hw_page);
    lv_obj_set_size(arc_ram, 122, 122);
    lv_obj_align(arc_ram, LV_ALIGN_CENTER, 66, -22);
    lv_arc_set_rotation(arc_ram, 135);
    lv_arc_set_bg_angles(arc_ram, 0, 270);
    lv_arc_set_range(arc_ram, 0, 100);
    lv_arc_set_value(arc_ram, 60);
    lv_obj_set_style_arc_width(arc_ram, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_ram, lv_color_hex(0x161E2C), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_ram, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_ram, lv_color_hex(0x00D2FF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_ram, true, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_ram, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_ram, LV_OBJ_FLAG_CLICKABLE);

    core_ram = lv_obj_create(hw_page);
    lv_obj_set_size(core_ram, 96, 96);
    lv_obj_align_to(core_ram, arc_ram, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(core_ram, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(core_ram, lv_color_hex(0x0C1018), 0);
    lv_obj_set_style_border_width(core_ram, 1, 0);
    lv_obj_set_style_border_color(core_ram, lv_color_hex(0x1E293B), 0);
    lv_obj_clear_flag(core_ram, LV_OBJ_FLAG_SCROLLABLE);

    lbl_ram_val = lv_label_create(core_ram);
    lv_label_set_text(lbl_ram_val, "60%");
    lv_obj_set_style_text_font(lbl_ram_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_ram_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_ram_val, LV_ALIGN_CENTER, 0, -8);

    lbl_ram_tag = lv_label_create(core_ram);
    lv_label_set_text(lbl_ram_tag, "RAM");
    lv_obj_set_style_text_font(lbl_ram_tag, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_ram_tag, lv_color_hex(0x00D2FF), 0);
    lv_obj_align(lbl_ram_tag, LV_ALIGN_CENTER, 0, 14);

    // 6. Network Telemetry Horizon Capsules (Image 3 & Image 4 style)
    cont_net = lv_obj_create(hw_page);
    lv_obj_set_size(cont_net, 280, 36);
    lv_obj_align(cont_net, LV_ALIGN_CENTER, 0, 62);
    lv_obj_set_style_bg_opa(cont_net, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_net, 0, 0);
    lv_obj_set_flex_flow(cont_net, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_net, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont_net, LV_OBJ_FLAG_SCROLLABLE);

    // Download Pill
    pill_net_down = lv_obj_create(cont_net);
    lv_obj_set_size(pill_net_down, 128, 30);
    lv_obj_set_style_radius(pill_net_down, 15, 0);
    lv_obj_set_style_bg_color(pill_net_down, lv_color_hex(0x101726), 0);
    lv_obj_set_style_border_width(pill_net_down, 1, 0);
    lv_obj_set_style_border_color(pill_net_down, lv_color_hex(0x1E2B42), 0);
    lv_obj_clear_flag(pill_net_down, LV_OBJ_FLAG_SCROLLABLE);

    lbl_net_down = lv_label_create(pill_net_down);
    lv_label_set_text(lbl_net_down, "D 0.0 MB/s");
    lv_obj_set_style_text_font(lbl_net_down, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_net_down, lv_color_hex(0x00D2FF), 0);
    lv_obj_center(lbl_net_down);

    // Upload Pill
    pill_net_up = lv_obj_create(cont_net);
    lv_obj_set_size(pill_net_up, 128, 30);
    lv_obj_set_style_radius(pill_net_up, 15, 0);
    lv_obj_set_style_bg_color(pill_net_up, lv_color_hex(0x101726), 0);
    lv_obj_set_style_border_width(pill_net_up, 1, 0);
    lv_obj_set_style_border_color(pill_net_up, lv_color_hex(0x1E2B42), 0);
    lv_obj_clear_flag(pill_net_up, LV_OBJ_FLAG_SCROLLABLE);

    lbl_net_up = lv_label_create(pill_net_up);
    lv_label_set_text(lbl_net_up, "U 0.0 MB/s");
    lv_obj_set_style_text_font(lbl_net_up, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_net_up, lv_color_hex(0x00FF88), 0);
    lv_obj_center(lbl_net_up);

    // 7. Backlight / Brightness Indicator Capsule (Smart Dial bottom style)
    pill_bri = lv_obj_create(hw_page);
    lv_obj_set_size(pill_bri, 160, 28);
    lv_obj_align(pill_bri, LV_ALIGN_CENTER, 0, 106);
    lv_obj_set_style_radius(pill_bri, 14, 0);
    lv_obj_set_style_bg_color(pill_bri, lv_color_hex(0x131924), 0);
    lv_obj_set_style_border_width(pill_bri, 1, 0);
    lv_obj_set_style_border_color(pill_bri, lv_color_hex(0x28354D), 0);
    lv_obj_clear_flag(pill_bri, LV_OBJ_FLAG_SCROLLABLE);

    lbl_bri = lv_label_create(pill_bri);
    lv_label_set_text(lbl_bri, "BACKLIGHT: 20%");
    lv_obj_set_style_text_font(lbl_bri, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_bri, lv_color_hex(0xFBBC05), 0);
    lv_obj_center(lbl_bri);
}

void page_hardware_set_brightness(uint8_t brightness_pct) {
    if (!lbl_bri) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "BACKLIGHT: %d%%", brightness_pct);
    lv_label_set_text(lbl_bri, buf);
}

void page_hardware_update(uint8_t cpu_pct, uint8_t ram_pct, float net_down_mb, float net_up_mb) {
    if (!arc_cpu || !arc_ram || !lbl_cpu_val || !lbl_ram_val || !lbl_net_down || !lbl_net_up) return;

    // 1. Update CPU
    lv_arc_set_value(arc_cpu, cpu_pct);
    char buf_cpu[16];
    snprintf(buf_cpu, sizeof(buf_cpu), "%d%%", cpu_pct);
    lv_label_set_text(lbl_cpu_val, buf_cpu);

    if (cpu_pct > 80) {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0xFF3344), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(lbl_cpu_tag, lv_color_hex(0xFF3344), 0);
    } else if (cpu_pct > 50) {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0xFBBC05), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(lbl_cpu_tag, lv_color_hex(0xFBBC05), 0);
    } else {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x00FF88), LV_PART_INDICATOR);
        lv_obj_set_style_text_color(lbl_cpu_tag, lv_color_hex(0x00FF88), 0);
    }

    // 2. Update RAM
    lv_arc_set_value(arc_ram, ram_pct);
    char buf_ram[16];
    snprintf(buf_ram, sizeof(buf_ram), "%d%%", ram_pct);
    lv_label_set_text(lbl_ram_val, buf_ram);

    // 3. Update Network Capsules
    char buf_down[32];
    snprintf(buf_down, sizeof(buf_down), "D %.1f MB/s", net_down_mb);
    lv_label_set_text(lbl_net_down, buf_down);

    char buf_up[32];
    snprintf(buf_up, sizeof(buf_up), "U %.1f MB/s", net_up_mb);
    lv_label_set_text(lbl_net_up, buf_up);
}
