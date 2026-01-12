#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Screen metrics shared with main (so overlay can reuse cached sizes) */
    lv_coord_t *p_screen_w;
    lv_coord_t *p_screen_h;
    lv_coord_t *p_screen_size;

    /* Callback: overlay depth tracking (must match your global swipe block rules) */
    void (*set_overlay)(bool open);

    /* Callback: update current mode string in your state model */
    void (*set_mode)(const char *mode_str);

    /* Callback: request a UI refresh when mode changes (you decide what to redraw) */
    void (*request_refresh)(void);

    /* ===== FX list provider (optional) =====
     * If any of these are NULL -> overlay uses internal fallback list.
     *
     * index: 0..count-1 (list order as provided by cache)
     * effect_id: stable ID used by Lamp firmware (what you send in SET_ANIM)
     */
    void *fx_arg;
    uint16_t (*fx_get_count)(void *arg);
    const char *(*fx_get_name)(void *arg, uint16_t index);
    uint16_t (*fx_get_id)(void *arg, uint16_t index);
    uint16_t (*fx_get_selected_id)(void *arg);


    /* Called when user confirms selection: pass EFFECT_ID (not index) */
    void (*fx_on_select)(void *arg, uint16_t effect_id);
} ui_anim_overlay_bind_t;

/* Call once after LVGL is ready and your screen metrics exist */
void ui_anim_overlay_init(const ui_anim_overlay_bind_t *bind);

void ui_anim_overlay_open(void);
void ui_anim_overlay_close(void);
bool ui_anim_overlay_is_open(void);
void ui_anim_overlay_open_for(void *fx_arg);


#ifdef __cplusplus
}
#endif
