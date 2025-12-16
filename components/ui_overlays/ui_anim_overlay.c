#include "ui_anim_overlay.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ===== Bind from main ===== */
static ui_anim_overlay_bind_t g_bind;
static bool g_inited = false;

/* ===== Local config (same values as in your main) ===== */
#define J_ANIM_POS_X          0
#define J_ANIM_POS_Y          0
#define J_DOUBLE_TAP_MS       350

#define J_ANIM_VISIBLE_ITEMS  7
#define J_ANIM_HALF_ITEMS     (J_ANIM_VISIBLE_ITEMS/2)

typedef enum {
    J_WHEEL_FONT_S = 0,
    J_WHEEL_FONT_M,
    J_WHEEL_FONT_L,
} j_wheel_font_profile_t;

/* One knob: discrete profile (fonts themselves are fixed) */
static float g_anim_wheel_scale = 1.00f;
static j_wheel_font_profile_t g_anim_wheel_font_profile = J_WHEEL_FONT_M;

static inline lv_coord_t j_scale_px_i(int v)
{
    int r = (int)lroundf((float)v * g_anim_wheel_scale);
    if (r < 1) r = 1;
    return (lv_coord_t)r;
}

static inline int32_t j_scale_i32(int32_t v)
{
    return (int32_t)lroundf((float)v * g_anim_wheel_scale);
}

static void anim_wheel_set_scale(float s)
{
    if (s < 0.70f) s = 0.70f;
    if (s > 1.60f) s = 1.60f;

    g_anim_wheel_scale = s;

    if (s < 0.90f) g_anim_wheel_font_profile = J_WHEEL_FONT_S;
    else if (s > 1.15f) g_anim_wheel_font_profile = J_WHEEL_FONT_L;
    else g_anim_wheel_font_profile = J_WHEEL_FONT_M;
}

/* Keep same list (source of truth for selector) */
static const char *g_animation_list[] = {
    "Ambient",
    "Aurora",
    "Neon",
    "Plasma",
    "Embers",
    "Ripple",
    "Comet",
    "Matrix",
    "Pulse",
    "Waves",
    "Glitch"
};
#define J_ANIM_COUNT ((int)(sizeof(g_animation_list)/sizeof(g_animation_list[0])))

static const struct {
    int32_t arc_radius;
    int32_t arc_x_offset;
    int32_t item_spacing;
    int32_t center_x_ofs;
    int32_t center_y_ofs;
    uint32_t snap_time_ms;
    float    inertia_strength;
    float    decel_per_s;
    float    vel_stop;
    int32_t  lock_px;
    int32_t  horiz_step_px;
    bool     cyclic;
    uint32_t color_active_hex;
    uint32_t color_inactive_hex;
} J_ANIM_CFG = {
    .arc_radius        = 190,
    .arc_x_offset      = -40,
    .item_spacing      = 34,
    .center_x_ofs      = 0,
    .center_y_ofs      = 0,
    .snap_time_ms      = 160,
    .inertia_strength  = 1.25f,
    .decel_per_s       = 5.0f,
    .vel_stop          = 0.35f,
    .lock_px           = 10,
    .horiz_step_px     = 26,
    .cyclic            = true,
    .color_active_hex  = 0x40D0FF,
    .color_inactive_hex= 0xE0E0E0,
};

/* ===== Local LVGL handles/state ===== */
static lv_obj_t  *s_overlay = NULL;
static lv_obj_t  *s_area = NULL;
static lv_obj_t  *s_labels[J_ANIM_VISIBLE_ITEMS] = {0};

static float     s_pos = 0.0f;   /* continuous index */
static float     s_vel = 0.0f;   /* items/s */
static int       s_index = 0;    /* snapped selection */
static lv_coord_t s_scroll_step_px = 0;

static bool      s_dragging = false;
static bool      s_axis_locked = false;
static bool      s_lock_vertical = true;
static bool      s_dragged_far = false;
static lv_point_t s_p_down = {0};
static lv_point_t s_p_last = {0};
static uint32_t   s_t_last_ms = 0;

static uint32_t   s_last_click_ms = 0;

static lv_timer_t *s_inertia_timer = NULL;

typedef struct {
    float from;
    float to;
} snap_ctx_t;

static snap_ctx_t s_snap_ctx;
static int s_snap_target = 0;

/* ===== Small local helpers (no dependency on ui_core) ===== */
static void make_invisible_hit_area(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

static inline void lv_obj_del_safe(lv_obj_t **pp)
{
    if (!pp) return;
    if (*pp) {
        lv_obj_del(*pp);
        *pp = NULL;
    }
}

static int wrap_index(int idx)
{
    if (J_ANIM_COUNT <= 0) return 0;
    int r = idx % J_ANIM_COUNT;
    if (r < 0) r += J_ANIM_COUNT;
    return r;
}

static int clamp_index(int idx)
{
    if (J_ANIM_COUNT <= 0) return 0;
    if (idx < 0) return 0;
    if (idx >= J_ANIM_COUNT) return (J_ANIM_COUNT - 1);
    return idx;
}

/* Arc bending left: x = -(R - sqrt(R^2 - y^2)) + arc_x_offset */
static int32_t arc_x_from_y(int32_t y_rel)
{
    int32_t R = j_scale_i32(J_ANIM_CFG.arc_radius);
    int32_t y = y_rel;
    if (y > R) y = R;
    if (y < -R) y = -R;

    float yf = (float)y;
    float Rf = (float)R;
    float inside = (Rf * Rf) - (yf * yf);
    if (inside < 0.0f) inside = 0.0f;

    float dx = Rf - sqrtf(inside);
    return (int32_t)(-dx) + j_scale_i32(J_ANIM_CFG.arc_x_offset);
}

/* Fonts: keep it generic. You can later swap to your Remote_Fonts.h mapping.
   For now use LVGL default fonts to avoid dependency surprises. */
static const lv_font_t *font_for_level(int lvl)
{
    /* lvl: 0 center, 1 next, 2 next, 3 far */
    switch (g_anim_wheel_font_profile) {
    case J_WHEEL_FONT_S:
        return (lvl <= 1) ? &lv_font_montserrat_22 : &lv_font_montserrat_18;
    case J_WHEEL_FONT_L:
        return (lvl == 0) ? &lv_font_montserrat_34 :
               (lvl == 1) ? &lv_font_montserrat_28 :
               (lvl == 2) ? &lv_font_montserrat_22 : &lv_font_montserrat_18;
    case J_WHEEL_FONT_M:
    default:
        return (lvl == 0) ? &lv_font_montserrat_28 :
               (lvl == 1) ? &lv_font_montserrat_22 :
               (lvl == 2) ? &lv_font_montserrat_18 : &lv_font_montserrat_14;
    }
}

static lv_coord_t text_h_for(const char *txt, const lv_font_t *f)
{
    if (!txt || !f) return 16;

    lv_point_t sz = {0};
    lv_txt_get_size(&sz, txt, f, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (sz.y <= 0) sz.y = (lv_coord_t)lv_font_get_line_height(f);
    return sz.y;
}

/* ===== Forward decls ===== */
static void selector_update(void);
static void selector_apply(int new_index);
static void selector_snap(bool animate);
static void selector_build(void);

static void snap_ready_cb(lv_anim_t *a)
{
    (void)a;
    selector_apply(s_snap_target);
    selector_update();
}

static void snap_exec_cb(void *var, int32_t v)
{
    (void)var;
    float t = (float)v / 1024.0f;
    s_pos = s_snap_ctx.from + (s_snap_ctx.to - s_snap_ctx.from) * t;
    selector_update();
}

static void selector_apply(int new_index)
{
    if (J_ANIM_COUNT <= 0) return;

    int idx = J_ANIM_CFG.cyclic ? wrap_index(new_index) : clamp_index(new_index);
    s_index = idx;
    s_pos = (float)idx;

    if (g_bind.set_mode) g_bind.set_mode(g_animation_list[idx]);
    if (g_bind.request_refresh) g_bind.request_refresh();
}

static void selector_update(void)
{
    if (!s_overlay || !s_area) return;
    if (J_ANIM_COUNT <= 0) return;

    int32_t area_w = lv_obj_get_width(s_area);
    int32_t area_h = lv_obj_get_height(s_area);

    int32_t cx = (area_w / 2) + J_ANIM_POS_X + J_ANIM_CFG.center_x_ofs;
    int32_t cy = (area_h / 2) + J_ANIM_POS_Y + J_ANIM_CFG.center_y_ofs;

    int base = (int)floorf(s_pos);
    float frac = s_pos - (float)base;

    lv_color_t c_active   = lv_color_hex(J_ANIM_CFG.color_active_hex);
    lv_color_t c_inactive = lv_color_hex(J_ANIM_CFG.color_inactive_hex);

    const lv_coord_t GAP_PX = j_scale_px_i(6);

    int idx0 = J_ANIM_CFG.cyclic ? wrap_index(base) : clamp_index(base);
    int idx1 = J_ANIM_CFG.cyclic ? wrap_index(base + 1) : clamp_index(base + 1);

    const char *t0 = g_animation_list[idx0];
    const char *t1 = g_animation_list[idx1];

    lv_coord_t h0 = text_h_for(t0, font_for_level(0));
    lv_coord_t h1 = text_h_for(t1, font_for_level(1));
    lv_coord_t step_px = (h0 / 2) + GAP_PX + (h1 / 2);
    if (step_px <= 1) step_px = j_scale_px_i(J_ANIM_CFG.item_spacing);

    s_scroll_step_px = step_px;

    for (int i = -J_ANIM_HALF_ITEMS; i <= J_ANIM_HALF_ITEMS; i++) {
        int slot = i + J_ANIM_HALF_ITEMS;
        lv_obj_t *lab = s_labels[slot];
        if (!lab) continue;

        int idx_raw = base + i;
        int idx = J_ANIM_CFG.cyclic ? wrap_index(idx_raw) : clamp_index(idx_raw);

        int lvl = (i < 0) ? -i : i;
        if (lvl > 3) lvl = 3;

        const char *txt = g_animation_list[idx];
        const lv_font_t *font = font_for_level(lvl);

        lv_label_set_text(lab, txt);
        lv_obj_set_style_text_font(lab, font, 0);
        lv_obj_set_style_text_color(lab, (i == 0) ? c_active : c_inactive, 0);

        if (lvl == 3)      lv_obj_set_style_text_opa(lab, LV_OPA_60, 0);
        else if (lvl == 2) lv_obj_set_style_text_opa(lab, LV_OPA_80, 0);
        else               lv_obj_set_style_text_opa(lab, LV_OPA_COVER, 0);

        lv_coord_t y = (lv_coord_t)cy;
        if (i != 0) {
            int dir = (i > 0) ? 1 : -1;
            int steps = (i > 0) ? i : -i;

            lv_coord_t h_prev = h0;
            for (int s = 1; s <= steps; s++) {
                int lvl_cur = s;
                if (lvl_cur > 3) lvl_cur = 3;

                int idx_cur_raw = base + (dir * s);
                int idx_cur = J_ANIM_CFG.cyclic ? wrap_index(idx_cur_raw) : clamp_index(idx_cur_raw);

                const char *tcur = g_animation_list[idx_cur];
                const lv_font_t *fcur = font_for_level(lvl_cur);
                lv_coord_t h_cur = text_h_for(tcur, fcur);

                y += dir * ((h_prev / 2) + GAP_PX + (h_cur / 2));
                h_prev = h_cur;
            }
        }

        y -= (lv_coord_t)lroundf(frac * (float)step_px);

        int32_t y_rel = (int32_t)y - (int32_t)cy;
        int32_t x = cx + arc_x_from_y(y_rel);

        lv_coord_t ht = text_h_for(txt, font);
        lv_obj_set_pos(lab, (lv_coord_t)x, (lv_coord_t)(y - (ht / 2)));
    }
}

static void selector_snap(bool animate)
{
    if (J_ANIM_COUNT <= 0) return;

    int target = (int)lroundf(s_pos);
    target = J_ANIM_CFG.cyclic ? wrap_index(target) : clamp_index(target);

    if (!animate) {
        selector_apply(target);
        selector_update();
        return;
    }

    s_snap_ctx.from = s_pos;
    s_snap_ctx.to   = (float)target;
    s_snap_target   = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);
    lv_anim_set_exec_cb(&a, snap_exec_cb);
    lv_anim_set_values(&a, 0, 1024);
    lv_anim_set_time(&a, J_ANIM_CFG.snap_time_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, snap_ready_cb);
    lv_anim_start(&a);
}

static void inertia_timer_cb(lv_timer_t *t)
{
    (void)t;

    static uint32_t last_ms = 0;
    uint32_t now = lv_tick_get();
    if (last_ms == 0) last_ms = now;

    float dt = (float)(now - last_ms) / 1000.0f;
    if (dt < 0.001f) dt = 0.001f;
    last_ms = now;

    s_pos += s_vel * dt;
    if (J_ANIM_CFG.cyclic && J_ANIM_COUNT > 0) {
        while (s_pos < 0.0f) s_pos += (float)J_ANIM_COUNT;
        while (s_pos >= (float)J_ANIM_COUNT) s_pos -= (float)J_ANIM_COUNT;
    } else {
        if (s_pos < 0.0f) { s_pos = 0.0f; s_vel = 0.0f; }
        if (s_pos > (float)(J_ANIM_COUNT - 1)) { s_pos = (float)(J_ANIM_COUNT - 1); s_vel = 0.0f; }
    }

    float k = 1.0f - (J_ANIM_CFG.decel_per_s * dt);
    if (k < 0.05f) k = 0.05f;
    if (k > 1.0f)  k = 1.0f;
    s_vel *= k;

    selector_update();

    if (fabsf(s_vel) < J_ANIM_CFG.vel_stop) {
        s_vel = 0.0f;
        if (s_inertia_timer) lv_timer_pause(s_inertia_timer);
        last_ms = 0;
        selector_snap(true);
    }
}

static void selector_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    const int lock_px = j_scale_px_i(J_ANIM_CFG.lock_px);

    if (code == LV_EVENT_PRESSED) {
        s_dragging = true;
        s_axis_locked = false;
        s_lock_vertical = true;
        s_dragged_far = false;

        s_p_down = p;
        s_p_last = p;
        s_t_last_ms = lv_tick_get();

        s_vel = 0.0f;
        if (s_inertia_timer) lv_timer_pause(s_inertia_timer);
    }
    else if (code == LV_EVENT_PRESSING && s_dragging) {
        int dx = p.x - s_p_down.x;
        int dy = p.y - s_p_down.y;

        if (!s_axis_locked) {
            if (abs(dx) > lock_px || abs(dy) > lock_px) {
                s_axis_locked = true;
                s_lock_vertical = (abs(dy) >= abs(dx));
            }
        }

        int step_dy = p.y - s_p_last.y;

        uint32_t now = lv_tick_get();
        float dt = (float)(now - s_t_last_ms) / 1000.0f;
        if (dt < 0.001f) dt = 0.001f;

        if (s_axis_locked && s_lock_vertical) {
            lv_coord_t step_px = (s_scroll_step_px > 0) ? s_scroll_step_px : (lv_coord_t)J_ANIM_CFG.item_spacing;
            if (step_px <= 0) step_px = 1;

            float delta_items = (float)(-step_dy) / (float)step_px;
            s_pos += delta_items;

            float v_inst = (delta_items / dt) * J_ANIM_CFG.inertia_strength;
            s_vel = s_vel * 0.70f + v_inst * 0.30f;

            selector_update();

            if (abs(dy) > (lock_px * 2)) s_dragged_far = true;
        } else {
            if (abs(dx) > (lock_px * 2)) s_dragged_far = true;
        }

        s_p_last = p;
        s_t_last_ms = now;
    }
    else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && s_dragging) {
        s_dragging = false;

        int total_dx = p.x - s_p_down.x;

        if (s_axis_locked && !s_lock_vertical) {
            int horiz_step_px = j_scale_px_i(J_ANIM_CFG.horiz_step_px);
            if (abs(total_dx) >= horiz_step_px) {
                if (total_dx < 0) selector_apply(s_index - 1);
                else              selector_apply(s_index + 1);
                selector_snap(true);
            } else {
                selector_snap(true);
            }
        } else {
            if (fabsf(s_vel) > J_ANIM_CFG.vel_stop) {
                if (s_inertia_timer) lv_timer_resume(s_inertia_timer);
            } else {
                selector_snap(true);
            }
        }

        if (s_dragged_far) lv_event_stop_bubbling(e);
    }
}

static void selector_build(void)
{
    if (!s_overlay) return;

    lv_coord_t w = (g_bind.p_screen_w) ? *g_bind.p_screen_w : lv_disp_get_hor_res(lv_disp_get_default());
    lv_coord_t h = (g_bind.p_screen_h) ? *g_bind.p_screen_h : lv_disp_get_ver_res(lv_disp_get_default());

    s_area = lv_obj_create(s_overlay);
    lv_obj_set_style_bg_opa(s_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_area, 0, 0);
    lv_obj_set_style_outline_width(s_area, 0, 0);
    lv_obj_set_style_shadow_width(s_area, 0, 0);
    lv_obj_set_style_pad_all(s_area, 0, 0);
    lv_obj_set_style_radius(s_area, 0, 0);

    lv_obj_set_size(s_area, w, (lv_coord_t)(h * 0.62f));
    lv_obj_align(s_area, LV_ALIGN_CENTER, J_ANIM_POS_X, J_ANIM_POS_Y);
    make_invisible_hit_area(s_area);
    lv_obj_add_flag(s_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_area, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_add_event_cb(s_area, selector_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_area, selector_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_area, selector_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_area, selector_event_cb, LV_EVENT_PRESS_LOST, NULL);

    for (int i = 0; i < J_ANIM_VISIBLE_ITEMS; i++) {
        s_labels[i] = lv_label_create(s_area);
        lv_obj_set_style_text_align(s_labels[i], LV_TEXT_ALIGN_LEFT, 0);
        lv_label_set_long_mode(s_labels[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_width(s_labels[i], w);
        lv_label_set_text(s_labels[i], "...");
    }

    if (!s_inertia_timer) {
        s_inertia_timer = lv_timer_create(inertia_timer_cb, 16, NULL);
        lv_timer_pause(s_inertia_timer);
    } else {
        lv_timer_pause(s_inertia_timer);
    }

    selector_apply(0);
    selector_update();
}

/* Overlay click: double tap closes */
static void overlay_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    uint32_t now = lv_tick_get();
    if (s_last_click_ms != 0 && lv_tick_elaps(s_last_click_ms) < J_DOUBLE_TAP_MS) {
        s_last_click_ms = 0;
        ui_anim_overlay_close();
    } else {
        s_last_click_ms = now;
    }
}

/* ===== Public API ===== */
void ui_anim_overlay_init(const ui_anim_overlay_bind_t *bind)
{
    if (!bind) return;
    g_bind = *bind;
    g_inited = true;
}

bool ui_anim_overlay_is_open(void)
{
    return (s_overlay != NULL);
}

void ui_anim_overlay_open(void)
{
    if (!g_inited) return;
    if (s_overlay) return;

    if (g_bind.set_overlay) g_bind.set_overlay(true);

    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        if (g_bind.set_overlay) g_bind.set_overlay(false);
        return;
    }

    /* Ensure cached screen metrics exist */
    if (g_bind.p_screen_w && g_bind.p_screen_h && g_bind.p_screen_size) {
        if (*g_bind.p_screen_w == 0 || *g_bind.p_screen_h == 0 || *g_bind.p_screen_size == 0) {
            lv_coord_t w = lv_disp_get_hor_res(disp);
            lv_coord_t h = lv_disp_get_ver_res(disp);
            *g_bind.p_screen_w = w;
            *g_bind.p_screen_h = h;
            *g_bind.p_screen_size = (w < h) ? w : h;
        }
    }

    lv_coord_t w = (g_bind.p_screen_w) ? *g_bind.p_screen_w : lv_disp_get_hor_res(disp);
    lv_coord_t h = (g_bind.p_screen_h) ? *g_bind.p_screen_h : lv_disp_get_ver_res(disp);

    lv_obj_t *top = lv_layer_top();
    if (!top) {
        if (g_bind.set_overlay) g_bind.set_overlay(false);
        return;
    }

    s_overlay = lv_obj_create(top);
    if (!s_overlay) {
        if (g_bind.set_overlay) g_bind.set_overlay(false);
        return;
    }

    lv_obj_set_size(s_overlay, w, h);
    lv_obj_center(s_overlay);

    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_outline_width(s_overlay, 0, 0);
    lv_obj_set_style_shadow_width(s_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_overlay, 0, 0);
    lv_obj_set_style_radius(s_overlay, 0, 0);

    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_overlay, overlay_event_cb, LV_EVENT_CLICKED, NULL);

    selector_build();

    /* Keep your previous choice: scale up wheel */
    anim_wheel_set_scale(1.30f);

    lv_obj_update_layout(s_overlay);
    if (s_area) lv_obj_update_layout(s_area);

#if LVGL_VERSION_MAJOR >= 8
    lv_refr_now(lv_disp_get_default());
#endif

    selector_update();
}

void ui_anim_overlay_close(void)
{
    if (!g_inited) return;
    if (!s_overlay) return;

    if (s_inertia_timer) lv_timer_pause(s_inertia_timer);

    s_vel = 0.0f;
    s_dragging = false;
    s_axis_locked = false;
    s_lock_vertical = true;
    s_dragged_far = false;
    s_t_last_ms = 0;
    s_last_click_ms = 0;

    s_area = NULL;
    for (int i = 0; i < J_ANIM_VISIBLE_ITEMS; i++) s_labels[i] = NULL;

    if (g_bind.set_overlay) g_bind.set_overlay(false);

    lv_obj_del_safe(&s_overlay);
}
