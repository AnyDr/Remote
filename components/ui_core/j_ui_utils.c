#include "j_ui_utils.h"

void make_invisible_hit_area(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

void j_lv_obj_del_safe(lv_obj_t **pp)
{
    if (!pp) return;
    if (*pp) {
        lv_obj_del(*pp);
        *pp = NULL;
    }
}

void jinny_apply_label_cfg(lv_obj_t *label, const j_label_cfg_t *cfg)
{
    if (!label || !cfg) return;

    lv_obj_set_style_text_font(label, cfg->font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(cfg->color_hex), 0);
    lv_obj_set_style_text_align(label, cfg->text_align, 0);
    lv_obj_align(label, cfg->align, cfg->ofs_x, cfg->ofs_y);
}
