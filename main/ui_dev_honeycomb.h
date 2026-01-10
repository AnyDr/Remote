#pragma once

#include <stdint.h>
#include "lvgl.h"

typedef struct {
    /* Global swipe handler (required) */
    lv_event_cb_t swipe_cb;

    /* Panel callbacks (optional, can be NULL) */
    lv_event_cb_t center_cb;
    lv_event_cb_t bottom_cb;

    /* Arc touch areas (optional, can be NULL) */
    lv_event_cb_t brightness_idle_cb;
    lv_event_cb_t speed_idle_cb;

    /* Fonts (optional) */
    const lv_font_t *font_name;
    const lv_font_t *font_mode;
    const lv_font_t *font_bottom;
    const lv_font_t *font_hint;

    /* Colors (optional) */
    lv_color_t bg_color;
    lv_color_t text_color;
    lv_color_t panel_bg_color;
    lv_color_t panel_border_color;
    lv_color_t speed_arc_color;

    /* Text (optional) */
    const char *title_text; /* center mode string initial */

    /* Arc geometry */
    int16_t bright_start;
    int16_t bright_end;
    int16_t speed_start;
    int16_t speed_end;
} ui_dev_honeycomb_cfg_t;

/* Create HoneyComb screen (Lamp-like, without top room panel) */
lv_obj_t *ui_dev_honeycomb_create(const ui_dev_honeycomb_cfg_t *cfg);

/* Setters (safe to call even if screen not created yet) */
void ui_dev_honeycomb_set_center_text(const char *name, const char *mode);
void ui_dev_honeycomb_set_center_colors(lv_color_t name_color, lv_color_t mode_color);
void ui_dev_honeycomb_set_bottom_text(const char *left, const char *right);

void ui_dev_honeycomb_set_arc_percent(int16_t brightness_percent, int16_t speed_percent);
void ui_dev_honeycomb_set_arc_colors(lv_color_t brightness_color, lv_color_t speed_color);
