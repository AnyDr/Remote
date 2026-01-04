#include "ui_dev_honeycomb.h"

#include "Remote_Fonts.h"
#include "Remote_UI_Layout.h"
#include "j_ui_utils.h"
#include "esp_log.h"





static const char *TAG_HC = "HC";


/* Single-instance UI handles (OK for now: one HoneyComb device) */
static lv_obj_t *s_scr                = NULL;

static lv_obj_t *s_center_container   = NULL;
static lv_obj_t *s_label_name         = NULL;
static lv_obj_t *s_label_mode         = NULL;

static lv_obj_t *s_bottom_container   = NULL;
static lv_obj_t *s_label_temp         = NULL;
static lv_obj_t *s_label_power        = NULL;

static lv_obj_t *s_brightness_arc     = NULL;
static lv_obj_t *s_speed_arc          = NULL;

static lv_obj_t *s_brightness_touch   = NULL;
static lv_obj_t *s_speed_touch        = NULL;

static lv_obj_t *s_left_hint          = NULL;
static lv_obj_t *s_right_hint         = NULL;

/* Cached arc parameters */
static int16_t s_bright_start = 210;
static int16_t s_bright_end   = 330;
static int16_t s_speed_start  = 30;
static int16_t s_speed_end    = 150;

static lv_color_t s_speed_arc_color = {0};

static inline void clamp_percent(int16_t *p)
{
    if (!p) return;
    if (*p < 0) *p = 0;
    if (*p > 100) *p = 100;
}

lv_obj_t *ui_dev_honeycomb_create(const ui_dev_honeycomb_cfg_t *cfg)
{
    lv_disp_t *disp = lv_disp_get_default();

    ESP_LOGI(TAG_HC, "create: file=%s cfg=%p title_text=%s",
             __FILE__,
             (void*)cfg,
             (cfg && cfg->title_text) ? cfg->title_text : "(null)");

    if (!disp) {
        LV_LOG_ERROR("ui_dev_honeycomb_create: no default display");
        return NULL;
    }

    const lv_coord_t w = lv_disp_get_hor_res(disp);
    const lv_coord_t h = lv_disp_get_ver_res(disp);
    const lv_coord_t screen_size = (w < h) ? w : h;

    /* дальше оставь твой текущий код как был */


    /* Create screen root */
    s_scr = lv_obj_create(NULL);
    if (!s_scr) return NULL;

    lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_outline_width(s_scr, 0, 0);
    lv_obj_set_size(s_scr, w, h);
    lv_obj_center(s_scr);

    const lv_color_t bg   = (cfg) ? cfg->bg_color   : lv_color_hex(0x0A0A0A);
    const lv_color_t text = (cfg) ? cfg->text_color : lv_color_hex(0xE0E0E0);

    lv_obj_set_style_bg_color(s_scr, bg, 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(s_scr, text, 0);

    /* Swipe handler on screen */
    if (cfg && cfg->swipe_cb) {
        lv_obj_add_event_cb(s_scr, cfg->swipe_cb, LV_EVENT_PRESSED,    NULL);
        lv_obj_add_event_cb(s_scr, cfg->swipe_cb, LV_EVENT_PRESSING,   NULL);
        lv_obj_add_event_cb(s_scr, cfg->swipe_cb, LV_EVENT_RELEASED,   NULL);
        lv_obj_add_event_cb(s_scr, cfg->swipe_cb, LV_EVENT_PRESS_LOST, NULL);
    }

    /* Cache arc angles */
    if (cfg) {
        s_bright_start = cfg->bright_start;
        s_bright_end   = cfg->bright_end;
        s_speed_start  = cfg->speed_start;
        s_speed_end    = cfg->speed_end;
        s_speed_arc_color = (cfg->speed_arc_color.full == 0) ? lv_color_hex(0x4080FF) : cfg->speed_arc_color;
    } else {
        s_speed_arc_color = lv_color_hex(0x4080FF);
    }

    /* Arcs (non-clickable, same as Lamp) */
    const lv_coord_t arc_size = (lv_coord_t)(screen_size * ARC_SIZE_RATIO);

    s_brightness_arc = lv_arc_create(s_scr);
    lv_obj_set_size(s_brightness_arc, arc_size, arc_size);
    lv_obj_center(s_brightness_arc);
    lv_arc_set_bg_angles(s_brightness_arc, s_bright_start, s_bright_end);
    lv_obj_remove_style(s_brightness_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_brightness_arc, 8,  LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_brightness_arc, 10, LV_PART_INDICATOR);
    lv_obj_move_background(s_brightness_arc);
    lv_obj_clear_flag(s_brightness_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_brightness_arc, LV_OBJ_FLAG_SCROLLABLE);

    s_speed_arc = lv_arc_create(s_scr);
    lv_obj_set_size(s_speed_arc, arc_size, arc_size);
    lv_obj_center(s_speed_arc);
    lv_arc_set_bg_angles(s_speed_arc, s_speed_start, s_speed_end);
    lv_obj_remove_style(s_speed_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_speed_arc, 8,  LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_speed_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_speed_arc, s_speed_arc_color, LV_PART_INDICATOR);
    lv_obj_move_background(s_speed_arc);
    lv_obj_clear_flag(s_speed_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_speed_arc, LV_OBJ_FLAG_SCROLLABLE);
        /* Make indicators visible even before first update call */
    if (s_brightness_arc) {
        const int16_t a = s_bright_start + (s_bright_end - s_bright_start) * 30 / 100;
        lv_arc_set_angles(s_brightness_arc, s_bright_start, a);
    }
    if (s_speed_arc) {
        const int16_t a = s_speed_start + (s_speed_end - s_speed_start) * 20 / 100;
        lv_arc_set_angles(s_speed_arc, s_speed_start, a);
    }


    /* Center panel */
    const lv_color_t panel_bg     = (cfg) ? cfg->panel_bg_color     : lv_color_hex(0x101010);
    const lv_color_t panel_border = (cfg) ? cfg->panel_border_color : lv_color_hex(0x404040);

    s_center_container = lv_obj_create(s_scr);
    lv_obj_set_size(s_center_container,
                    (lv_coord_t)(screen_size * CENTER_W_RATIO),
                    (lv_coord_t)(screen_size * CENTER_H_RATIO));
    lv_obj_center(s_center_container);

    lv_obj_set_style_bg_opa(s_center_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(s_center_container, CENTER_BORDER_WIDTH, 0);
    lv_obj_set_style_border_color(s_center_container, panel_border, 0);
    lv_obj_set_style_radius(s_center_container, (lv_coord_t)(screen_size * CENTER_RADIUS_RATIO), 0);
    lv_obj_set_style_pad_all(s_center_container, 8, 0);
    lv_obj_clear_flag(s_center_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_center_container, LV_OBJ_FLAG_EVENT_BUBBLE);

    if (cfg && cfg->center_cb) {
        lv_obj_add_event_cb(s_center_container, cfg->center_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(s_center_container, cfg->center_cb, LV_EVENT_LONG_PRESSED, NULL);
    }

    s_label_name = lv_label_create(s_center_container);
    lv_label_set_text(s_label_name, "HoneyComb");
    if (cfg && cfg->font_name) lv_obj_set_style_text_font(s_label_name, cfg->font_name, 0);
    lv_obj_set_style_text_color(s_label_name, text, 0);
    lv_obj_set_style_text_align(s_label_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_label_name, LV_ALIGN_TOP_MID, 0, 0);


    s_label_mode = lv_label_create(s_center_container);
    lv_label_set_text(s_label_mode, (cfg && cfg->title_text) ? cfg->title_text : "stub");
    if (cfg && cfg->font_mode) lv_obj_set_style_text_font(s_label_mode, cfg->font_mode, 0);
    lv_obj_set_style_text_color(s_label_mode, text, 0);
    lv_obj_set_style_text_align(s_label_mode, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_label_mode, LV_ALIGN_BOTTOM_MID, 0, -4);


    /* Bottom panel (no top room panel by design) */
    s_bottom_container = lv_obj_create(s_scr);
    lv_obj_set_size(s_bottom_container,
                    (lv_coord_t)(screen_size * DEV_W_RATIO),
                    (lv_coord_t)(screen_size * DEV_H_RATIO));
    lv_obj_align(s_bottom_container, LV_ALIGN_BOTTOM_MID, 0,
                 -(lv_coord_t)(screen_size * DEV_Y_RATIO) + DEV_OFFSET_Y);

    lv_obj_set_style_radius(s_bottom_container, 12, 0);
    lv_obj_set_style_bg_opa(s_bottom_container, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(s_bottom_container, panel_bg, 0);
    lv_obj_set_style_border_width(s_bottom_container, 1, 0);
    lv_obj_set_style_border_color(s_bottom_container, panel_border, 0);
    lv_obj_set_style_pad_all(s_bottom_container, 4, 0);
    lv_obj_clear_flag(s_bottom_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_bottom_container, LV_OBJ_FLAG_EVENT_BUBBLE);

    if (cfg && cfg->bottom_cb) {
        lv_obj_add_event_cb(s_bottom_container, cfg->bottom_cb, LV_EVENT_CLICKED, NULL);
    }

    s_label_temp = lv_label_create(s_bottom_container);
    if (cfg && cfg->font_bottom) lv_obj_set_style_text_font(s_label_temp, cfg->font_bottom, 0);
    lv_obj_set_style_text_color(s_label_temp, text, 0);
    lv_obj_set_style_text_align(s_label_temp, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(s_label_temp, "Dev: --.-°C");
    lv_obj_align(s_label_temp, LV_ALIGN_LEFT_MID, 4, 0);

    s_label_power = lv_label_create(s_bottom_container);
    if (cfg && cfg->font_bottom) lv_obj_set_style_text_font(s_label_power, cfg->font_bottom, 0);
    lv_obj_set_style_text_color(s_label_power, text, 0);
    lv_obj_set_style_text_align(s_label_power, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(s_label_power, "--.- W");
    lv_obj_align(s_label_power, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Touch areas for arc overlays (reuse existing callbacks) */
    s_brightness_touch = lv_obj_create(s_scr);
    lv_obj_set_size(s_brightness_touch,
                    (lv_coord_t)(screen_size * ARC_TOUCH_W_RATIO),
                    (lv_coord_t)(screen_size * ARC_TOUCH_H_RATIO));
    lv_obj_align(s_brightness_touch, LV_ALIGN_TOP_MID, 0, (lv_coord_t)(screen_size * ARC_TOUCH_OFFSET));
    make_invisible_hit_area(s_brightness_touch);
    lv_obj_add_flag(s_brightness_touch, LV_OBJ_FLAG_EVENT_BUBBLE);
    if (cfg && cfg->brightness_idle_cb) {
        lv_obj_add_event_cb(s_brightness_touch, cfg->brightness_idle_cb, LV_EVENT_LONG_PRESSED, NULL);
    }

    s_speed_touch = lv_obj_create(s_scr);
    lv_obj_set_size(s_speed_touch,
                    (lv_coord_t)(screen_size * ARC_TOUCH_W_RATIO),
                    (lv_coord_t)(screen_size * ARC_TOUCH_H_RATIO));
    lv_obj_align(s_speed_touch, LV_ALIGN_BOTTOM_MID, 0, -(lv_coord_t)(screen_size * ARC_TOUCH_OFFSET));
    make_invisible_hit_area(s_speed_touch);
    lv_obj_add_flag(s_speed_touch, LV_OBJ_FLAG_EVENT_BUBBLE);
    if (cfg && cfg->speed_idle_cb) {
        lv_obj_add_event_cb(s_speed_touch, cfg->speed_idle_cb, LV_EVENT_LONG_PRESSED, NULL);
    }

    /* Left/Right hints */
    s_left_hint = lv_label_create(s_scr);
    lv_label_set_text(s_left_hint, LV_SYMBOL_LEFT);
    if (cfg && cfg->font_hint) lv_obj_set_style_text_font(s_left_hint, cfg->font_hint, 0);
    lv_obj_align(s_left_hint, LV_ALIGN_LEFT_MID, (lv_coord_t)(screen_size * HINT_OFFSET_RATIO), 0);
    lv_obj_add_flag(s_left_hint, LV_OBJ_FLAG_EVENT_BUBBLE);

    s_right_hint = lv_label_create(s_scr);
    lv_label_set_text(s_right_hint, LV_SYMBOL_RIGHT);
    if (cfg && cfg->font_hint) lv_obj_set_style_text_font(s_right_hint, cfg->font_hint, 0);
    lv_obj_align(s_right_hint, LV_ALIGN_RIGHT_MID, -(lv_coord_t)(screen_size * HINT_OFFSET_RATIO), 0);
    lv_obj_add_flag(s_right_hint, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* Foreground panels */
    lv_obj_move_foreground(s_center_container);
    lv_obj_move_foreground(s_bottom_container);

    return s_scr;
}
void ui_dev_honeycomb_set_center_text(const char *name, const char *mode)
{
    if (s_label_name && name) lv_label_set_text(s_label_name, name);
    if (s_label_mode && mode) lv_label_set_text(s_label_mode, mode);
}

void ui_dev_honeycomb_set_center_colors(lv_color_t name_color, lv_color_t mode_color)
{
    if (s_label_name) lv_obj_set_style_text_color(s_label_name, name_color, 0);
    if (s_label_mode) lv_obj_set_style_text_color(s_label_mode, mode_color, 0);
}

void ui_dev_honeycomb_set_bottom_text(const char *left, const char *right)
{
    if (s_label_temp && left)  lv_label_set_text(s_label_temp, left);
    if (s_label_power && right) lv_label_set_text(s_label_power, right);
}

void ui_dev_honeycomb_set_arc_percent(int16_t brightness_percent, int16_t speed_percent)
{
    clamp_percent(&brightness_percent);
    clamp_percent(&speed_percent);

    if (s_brightness_arc) {
        const int16_t a = s_bright_start + (s_bright_end - s_bright_start) * brightness_percent / 100;
        lv_arc_set_angles(s_brightness_arc, s_bright_start, a);
    }

    if (s_speed_arc) {
        const int16_t a = s_speed_start + (s_speed_end - s_speed_start) * speed_percent / 100;
        lv_arc_set_angles(s_speed_arc, s_speed_start, a);
    }
}

void ui_dev_honeycomb_set_arc_colors(lv_color_t brightness_color, lv_color_t speed_color)
{
    if (s_brightness_arc) lv_obj_set_style_arc_color(s_brightness_arc, brightness_color, LV_PART_INDICATOR);
    if (s_speed_arc)      lv_obj_set_style_arc_color(s_speed_arc, speed_color, LV_PART_INDICATOR);
}
