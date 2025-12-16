#pragma once

#include "lvgl.h"

typedef struct {
    lv_event_cb_t     swipe_cb;     /* global swipe handler (required for swipe) */
    const lv_font_t  *title_font;   /* optional */
    lv_color_t        bg_color;     /* optional */
    lv_color_t        text_color;   /* optional */
    const char       *title_text;   /* optional */
} ui_dev_honeycomb_cfg_t;

lv_obj_t *ui_dev_honeycomb_create(const ui_dev_honeycomb_cfg_t *cfg);
