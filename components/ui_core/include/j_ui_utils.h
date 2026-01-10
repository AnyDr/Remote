#pragma once

#include "lvgl.h"

#ifndef J_UNUSED
#define J_UNUSED(x) ((void)(x))
#endif

void make_invisible_hit_area(lv_obj_t *obj);
void j_lv_obj_del_safe(lv_obj_t **pp);

typedef struct {
    const lv_font_t   *font;
    uint32_t           color_hex;  // 0xRRGGBB
    lv_align_t         align;
    lv_text_align_t    text_align;
    lv_coord_t         ofs_x;
    lv_coord_t         ofs_y;
} j_label_cfg_t;

void jinny_apply_label_cfg(lv_obj_t *label, const j_label_cfg_t *cfg);
