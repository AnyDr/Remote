#include "ui_dev_honeycomb.h"

lv_obj_t *ui_dev_honeycomb_create(const ui_dev_honeycomb_cfg_t *cfg)
{
    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        LV_LOG_ERROR("ui_dev_honeycomb_create: no default display");
        return NULL;
    }

    const lv_coord_t w = lv_disp_get_hor_res(disp);
    const lv_coord_t h = lv_disp_get_ver_res(disp);

    lv_obj_t *scr = lv_obj_create(NULL);
    if (!scr) return NULL;

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_outline_width(scr, 0, 0);
    lv_obj_set_size(scr, w, h);
    lv_obj_center(scr);

    const lv_color_t bg   = (cfg) ? cfg->bg_color   : lv_color_hex(0x0A0A0A);
    const lv_color_t text = (cfg) ? cfg->text_color : lv_color_hex(0xE0E0E0);

    lv_obj_set_style_bg_color(scr, bg, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, text, 0);

    if (cfg && cfg->swipe_cb) {
        lv_obj_add_event_cb(scr, cfg->swipe_cb, LV_EVENT_PRESSED,   NULL);
        lv_obj_add_event_cb(scr, cfg->swipe_cb, LV_EVENT_PRESSING,  NULL);
        lv_obj_add_event_cb(scr, cfg->swipe_cb, LV_EVENT_RELEASED,  NULL);
        lv_obj_add_event_cb(scr, cfg->swipe_cb, LV_EVENT_PRESS_LOST,NULL);
    }

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, (cfg && cfg->title_text) ? cfg->title_text : "HoneyComb");
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    if (cfg && cfg->title_font) {
        lv_obj_set_style_text_font(title, cfg->title_font, 0);
    }

    lv_obj_center(title);

    return scr;
}
