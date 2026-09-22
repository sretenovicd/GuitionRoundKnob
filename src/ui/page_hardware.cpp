#include "page_hardware.h"

static lv_obj_t *hw_page = NULL;
static lv_obj_t *arc_cpu = NULL;
static lv_obj_t *lbl_cpu = NULL;
static lv_obj_t *arc_ram = NULL;
static lv_obj_t *lbl_ram = NULL;
static lv_obj_t *lbl_net = NULL;
static lv_obj_t *lbl_bri = NULL;

void page_hardware_create(lv_obj_t *parent) {
    hw_page = parent;
    lv_obj_set_style_bg_color(hw_page, lv_color_hex(0x0C121E), 0);

    // Title
    lv_obj_t *lbl_hdr = lv_label_create(hw_page);
    lv_label_set_text(lbl_hdr, "SYSTEM RESOURCES");
    lv_obj_set_style_text_font(lbl_hdr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_hdr, lv_color_hex(0x6A7B95), 0);
    lv_obj_align(lbl_hdr, LV_ALIGN_TOP_MID, 0, 30);

    // CPU Gauge (Left side)
    arc_cpu = lv_arc_create(hw_page);
    lv_obj_set_size(arc_cpu, 120, 120);
    lv_obj_align(arc_cpu, LV_ALIGN_CENTER, -65, -20);
    lv_arc_set_rotation(arc_cpu, 135);
    lv_arc_set_bg_angles(arc_cpu, 0, 270);
    lv_arc_set_range(arc_cpu, 0, 100);
    lv_arc_set_value(arc_cpu, 25);
    lv_obj_remove_style(arc_cpu, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_cpu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_cpu, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x1B2636), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_cpu, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x00FF88), LV_PART_INDICATOR);

    lbl_cpu = lv_label_create(hw_page);
    lv_label_set_text(lbl_cpu, "25%\nCPU");
    lv_obj_set_style_text_align(lbl_cpu, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_cpu, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_cpu, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(lbl_cpu, arc_cpu, LV_ALIGN_CENTER, 0, 0);

    // RAM Gauge (Right side)
    arc_ram = lv_arc_create(hw_page);
    lv_obj_set_size(arc_ram, 120, 120);
    lv_obj_align(arc_ram, LV_ALIGN_CENTER, 65, -20);
    lv_arc_set_rotation(arc_ram, 135);
    lv_arc_set_bg_angles(arc_ram, 0, 270);
    lv_arc_set_range(arc_ram, 0, 100);
    lv_arc_set_value(arc_ram, 60);
    lv_obj_remove_style(arc_ram, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_ram, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_ram, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_ram, lv_color_hex(0x1B2636), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_ram, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_ram, lv_color_hex(0x00B4D8), LV_PART_INDICATOR);

    lbl_ram = lv_label_create(hw_page);
    lv_label_set_text(lbl_ram, "60%\nRAM");
    lv_obj_set_style_text_align(lbl_ram, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_ram, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_ram, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(lbl_ram, arc_ram, LV_ALIGN_CENTER, 0, 0);

    // Network Speeds (Bottom)
    lbl_net = lv_label_create(hw_page);
    lv_label_set_text(lbl_net, "D: 0.0 MB/s  |  U: 0.0 MB/s");
    lv_obj_set_style_text_font(lbl_net, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net, lv_color_hex(0x90E0EF), 0);
    lv_obj_align(lbl_net, LV_ALIGN_CENTER, 0, 75);

    // Backlight / Brightness Indicator (Bottom)
    lbl_bri = lv_label_create(hw_page);
    lv_label_set_text(lbl_bri, "BACKLIGHT: 80%");
    lv_obj_set_style_text_font(lbl_bri, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_bri, lv_color_hex(0xFFA500), 0);
    lv_obj_align(lbl_bri, LV_ALIGN_CENTER, 0, 108);
}

void page_hardware_set_brightness(uint8_t brightness_pct) {
    if (!lbl_bri) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "BACKLIGHT: %d%%", brightness_pct);
    lv_label_set_text(lbl_bri, buf);
}

void page_hardware_update(uint8_t cpu_pct, uint8_t ram_pct, float net_down_mb, float net_up_mb) {
    if (!arc_cpu || !arc_ram || !lbl_cpu || !lbl_ram || !lbl_net) return;

    lv_arc_set_value(arc_cpu, cpu_pct);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%\nCPU", cpu_pct);
    lv_label_set_text(lbl_cpu, buf);

    // Color gradient for CPU
    if (cpu_pct > 80) {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0xFF3333), LV_PART_INDICATOR);
    } else if (cpu_pct > 50) {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0xFFB703), LV_PART_INDICATOR);
    } else {
        lv_obj_set_style_arc_color(arc_cpu, lv_color_hex(0x00FF88), LV_PART_INDICATOR);
    }

    lv_arc_set_value(arc_ram, ram_pct);
    snprintf(buf, sizeof(buf), "%d%%\nRAM", ram_pct);
    lv_label_set_text(lbl_ram, buf);

    snprintf(buf, sizeof(buf), "D: %.1f MB/s  |  U: %.1f MB/s", net_down_mb, net_up_mb);
    lv_label_set_text(lbl_net, buf);
}

