#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>   // sqrtf, atan2f, fabsf
#include <string.h> // strcmp
#include <stdlib.h> // abs


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "Display_SPD2010.h"
#include "PCF85063.h"
#include "QMI8658.h"
#include "SD_MMC.h"
#include "Wireless.h"
#include "TCA9554PWR.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "PWR_Key.h"
#include "PCM5101.h"
#include "MIC_Speech.h"
#include "Remote_Fonts.h"
#include "Remote_UI_Layout.h"
#include "esp_log.h"
#include "j_ui_utils.h"
#include "ui_anim_overlay.h"
#include "ui_dev_honeycomb.h"
#include "j_espnow_link.h"





/* ============================================================
 *                  BUILD / HYGIENE HELPERS
 * ============================================================*/


/* Compile-time assert (C11). If your toolchain is older, we’ll adjust. */
#ifndef J_STATIC_ASSERT
#define J_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif



/* ============================================================
 *                      CONSTANTS
 * ============================================================*/

#define BRIGHTNESS_ARC_START   210
#define BRIGHTNESS_ARC_END     330
#define SPEED_ARC_START        30
#define SPEED_ARC_END          150

#define J_COLOR_BG_MAIN        lv_color_hex(0x101820)
#define J_COLOR_BG_DIAG        lv_color_hex(0x080C14)
#define J_COLOR_TEXT_MAIN      lv_color_hex(0xE0E0E0)
#define J_COLOR_TEXT_SECONDARY lv_color_hex(0xC0C0C0)
#define J_COLOR_PANEL_BG       lv_color_hex(0x101010)
#define J_COLOR_PANEL_BORDER   lv_color_hex(0x404040)
#define J_COLOR_STATUS_OK      lv_color_hex(0x00FF80)
#define J_COLOR_STATUS_WARN    lv_color_hex(0xFFC000)
#define J_COLOR_STATUS_OFF     lv_color_hex(0xFF4040)
#define J_COLOR_ARC_SAFE       lv_color_hex(0x40FF40)
#define J_COLOR_ARC_WARN       lv_color_hex(0xFFD040)
#define J_COLOR_ARC_DANGER     lv_color_hex(0xFF4040)
#define J_COLOR_SPEED_ARC      lv_color_hex(0x4080FF)
#define J_MODE_COLOR_ACTIVE_HEX  0x40D0FF


#define J_BATT_ICON_W          34
#define J_BATT_ICON_H          16

#define J_BATT_POS_X           0
#define J_BATT_POS_Y           180
#define J_BATT_CONT_W          (J_BATT_ICON_W + 50)
#define J_BATT_CONT_H          26

/* Animation selector offset from screen center */
#define J_ANIM_POS_X    0     /* (-) left, (+) right */
#define J_ANIM_POS_Y    0     /* (-) up,   (+) down */

#define J_DOUBLE_TAP_MS        350

/* ============================================================
 *                HITBOX DEBUG CONFIG
 * ============================================================*/

#define J_DEBUG_HITBOXES_GLOBAL      0

#define J_DEBUG_HITBOX_CENTER        1
#define J_DEBUG_HITBOX_TOP_ROOM      1
#define J_DEBUG_HITBOX_BOTTOM_DEV    1
#define J_DEBUG_HITBOX_BRIGHTNESS    1
#define J_DEBUG_HITBOX_SPEED         1

#define J_HITBOX_OPA_AREA            LV_OPA_30
#define J_HITBOX_OPA_OUTLINE         LV_OPA_80
#define J_HITBOX_BORDER_WIDTH        1
#define J_HITBOX_OUTLINE_WIDTH       2

#define J_COLOR_HITBOX_CENTER        lv_color_hex(0xFF00FF)
#define J_COLOR_HITBOX_TOP           lv_color_hex(0x00FFFF)
#define J_COLOR_HITBOX_BOTTOM        lv_color_hex(0xFFFF00)
#define J_COLOR_HITBOX_BRIGHT        lv_color_hex(0x00FF00)
#define J_COLOR_HITBOX_SPEED         lv_color_hex(0xFF8000)

#define ARC_HIT_THICKNESS_RATIO      0.20f

#define J_DEBUG_ANIM_SELECTOR  0


/* ============================================================
 *                DEVICE STATE MODEL
 * ============================================================*/

typedef struct {
    const char *name;
    const char *mode;
    bool is_on;
    bool is_online;

    float room_temp;
    float room_humidity;

    float device_temp;
    float device_temp_max_today;
    float device_temp_max_ever;

    float power_w;
    float power_w_max;

    float energy_kwh;
    float psu_max_w;
    float lamp_theoretical_w;

    float energy_price_eur_per_kwh;   // €/kWh
} device_state_t;

static device_state_t g_current_device = {
    .name               = "Jinny`s Lamp",
    .mode               = "Ambient",
    .is_on              = true,
    .is_online          = true,
    .room_temp          = 22.5f,
    .room_humidity      = 45.0f,

    .device_temp        = 37.0f,
    .device_temp_max_today = 45.0f,
    .device_temp_max_ever  = 60.0f,

    .power_w            = 36.0f,
    .power_w_max        = 72.0f,

    .energy_kwh         = 0.42f,
    .psu_max_w          = 60.0f,
    .lamp_theoretical_w = 90.0f,

    .energy_price_eur_per_kwh = 0.30f
};

// ===== BEGIN PATCH: per-device state for HoneyComb =====
static device_state_t g_honey_device = {
    .name               = "HoneyComb",
    .mode               = "Ambient",
    .is_on              = true,
    .is_online          = true,

    .room_temp          = 22.5f,
    .room_humidity      = 45.0f,

    .device_temp        = 37.0f,
    .device_temp_max_today = 45.0f,
    .device_temp_max_ever  = 60.0f,

    .power_w            = 36.0f,
    .power_w_max        = 72.0f,

    .energy_kwh         = 0.42f,
    .psu_max_w          = 60.0f,
    .lamp_theoretical_w = 90.0f,

    .energy_price_eur_per_kwh = 0.30f
};
// ===== END PATCH =====


/* ============================================================
 *        ROOM VIEW MODES (TOP WINDOW)
 * ============================================================*/

typedef enum {
    ROOM_VIEW_CURRENT = 0,
    ROOM_VIEW_LAST_10H,
    ROOM_VIEW_LAST_24H,
    ROOM_VIEW_COUNT
} room_view_mode_t;

static room_view_mode_t g_room_view_mode = ROOM_VIEW_CURRENT;

/* ============================================================
 *   DEV TEMP / POWER VIEW MODES (BOTTOM WINDOW)
 * ============================================================*/

typedef enum {
    DEV_TEMP_VIEW_CURRENT = 0,
    DEV_TEMP_VIEW_MAX_TODAY,
    DEV_TEMP_VIEW_MAX_EVER,
    DEV_TEMP_VIEW_COUNT
} dev_temp_view_mode_t;

typedef enum {
    DEV_PWR_VIEW_CURRENT = 0,
    DEV_PWR_VIEW_MAX,
    DEV_PWR_VIEW_COST,
    DEV_PWR_VIEW_COUNT
} dev_pwr_view_mode_t;


static const j_label_cfg_t J_LABEL_CFG_NAME = {
    .font       = J_FONT_DEVICE_NAME,
    .color_hex  = 0xE0E0E0,
    .align      = LV_ALIGN_TOP_MID,
    .text_align = LV_TEXT_ALIGN_CENTER,
    .ofs_x      = 0,
    .ofs_y      = 0,
};

static const j_label_cfg_t J_LABEL_CFG_MODE = {
    .font       = J_FONT_DEVICE_NAME,          // крупнее
    .color_hex  = 0x40D0FF,                     // тот же, что в селекторе
    .align      = LV_ALIGN_BOTTOM_MID,
    .text_align = LV_TEXT_ALIGN_CENTER,
    .ofs_x      = 0,
    .ofs_y      = -4,                           // чуть приподнять
};


static const j_label_cfg_t J_LABEL_CFG_ROOM = {
    .font       = J_FONT_SMALL_TITLE,
    .color_hex  = 0xE0E0E0,
    .align      = LV_ALIGN_CENTER,
    .text_align = LV_TEXT_ALIGN_CENTER,
    .ofs_x      = 0,
    .ofs_y      = 0,
};

static const j_label_cfg_t J_LABEL_CFG_DEV_TEMP = {
    .font       = J_FONT_BODY,
    .color_hex  = 0xE0E0E0,
    .align      = LV_ALIGN_LEFT_MID,
    .text_align = LV_TEXT_ALIGN_LEFT,
    .ofs_x      = 4,
    .ofs_y      = 0,
};

static const j_label_cfg_t J_LABEL_CFG_DEV_POWER = {
    .font       = J_FONT_BODY,
    .color_hex  = 0xE0E0E0,
    .align      = LV_ALIGN_RIGHT_MID,
    .text_align = LV_TEXT_ALIGN_RIGHT,
    .ofs_x      = -4,
    .ofs_y      = 0,
};

/* ============================================================
 *                LVGL OBJECT HANDLES
 * ============================================================*/

static lv_obj_t *screen_device;
static lv_obj_t *screen_diag;
static lv_obj_t *screen_honeycomb;


static lv_obj_t *diag_label_title;
static lv_obj_t *diag_label_fps;
static lv_obj_t *diag_label_batt;
static lv_obj_t *diag_label_rtc;
static lv_obj_t *diag_label_heap;

static lv_obj_t *diag_label_conn;
static lv_obj_t *diag_label_wifi_rssi;
static lv_obj_t *diag_label_imu;
static lv_obj_t *diag_label_motion;

static lv_obj_t *diag_batt_container;
static lv_obj_t *diag_batt_inner;
static lv_obj_t *diag_batt_text;

/* Legacy screen array removed: navigation uses g_devs[] + j_dev_switch_to() */

static lv_obj_t *center_container;
static lv_obj_t *label_name;
static lv_obj_t *label_mode;

static lv_obj_t *top_room_container;
static lv_obj_t *label_room;

static lv_obj_t *bottom_dev_container;
static lv_obj_t *label_dev_temp;
static lv_obj_t *label_dev_power;

static lv_obj_t *brightness_arc;
static lv_obj_t *brightness_touch_area;

static lv_obj_t *speed_arc;
static lv_obj_t *speed_touch_area;

static lv_obj_t *left_hint;
static lv_obj_t *right_hint;

static lv_obj_t *brightness_overlay = NULL;
static lv_obj_t *speed_overlay      = NULL;

static lv_coord_t g_screen_w    = 0;
static lv_coord_t g_screen_h    = 0;
static lv_coord_t g_screen_size = 0;
static lv_coord_t g_arc_size    = 0;


static uint32_t g_center_last_click_ms = 0;
static uint32_t g_honey_center_last_click_ms = 0;


/* ============================================================
 *        DEVICE ARCH (STEP 1): TYPES ONLY, NO BEHAVIOR
 * ============================================================*/

/* Feature flag reserved for next steps (keep unused for now) */
#ifndef J_UI_DEVARCH_ENABLE
#define J_UI_DEVARCH_ENABLE 0
#endif

typedef struct j_dev_ctx j_dev_ctx_t;

typedef struct {
    const char *id;    /* stable identifier: "lamp", "system", ... */
    const char *name;  /* display name */

    /* Root screen lifecycle */
    lv_obj_t *(*create_root)(j_dev_ctx_t *d);
    void      (*destroy_root)(j_dev_ctx_t *d);

    /* Optional hooks (may be NULL) */
    void      (*on_enter)(j_dev_ctx_t *d);
    void      (*on_leave)(j_dev_ctx_t *d);
    void      (*refresh)(j_dev_ctx_t *d);
} j_dev_drv_t;

// ===== BEGIN PATCH: per-device UI state in ctx =====
struct j_dev_ctx {
    const j_dev_drv_t *drv;
    lv_obj_t          *root;

    /* Global swipe blocking conditions */
    uint8_t            stack_depth;   /* local submenu stack depth */
    uint8_t            overlay_depth; /* number of active overlays (0 = none) */

    device_state_t    *st;

    /* Per-device UI state (must NOT be global) */
    int16_t            brightness_percent; /* 0..100 */
    int16_t            speed_percent;      /* 0..100 */
    dev_temp_view_mode_t dev_temp_view_mode;
    dev_pwr_view_mode_t  dev_pwr_view_mode;
};
// ===== END PATCH =====



/* NOTE:
 * - Do not instantiate or use these yet (Step 2+).
 * - This block must compile cleanly with current code.
 */


/* ============================================================
 *        DEVICE ARCH (STEP 2): REGISTRY + MANAGER (UNUSED)
 * ============================================================*/

#define J_DEV_MAX  16

static j_dev_ctx_t g_devs[J_DEV_MAX] = {0};
static int         g_dev_count       = 0;
static int         g_active_dev_idx  = 0;

/* Helpers (unused in Step 2, wired in Step 4+) */
static inline j_dev_ctx_t *j_active_dev(void)
{
    if (g_dev_count <= 0) return NULL;
    if (g_active_dev_idx < 0) g_active_dev_idx = 0;
    if (g_active_dev_idx >= g_dev_count) g_active_dev_idx = g_dev_count - 1;
    return &g_devs[g_active_dev_idx];
}

// ===== BEGIN PATCH: find device ctx by root screen =====
static inline j_dev_ctx_t *j_dev_find_by_root(lv_obj_t *root)
{
    if (!root) return NULL;
    for (int i = 0; i < g_dev_count; i++) {
        if (g_devs[i].root == root) return &g_devs[i];
    }
    return NULL;
}
// ===== END PATCH =====


// ===== BEGIN PATCH: find device ctx by root =====
#if 0
static j_dev_ctx_t *j_dev_find_by_root(lv_obj_t *root)
{
    if (!root) return NULL;
    for (int i = 0; i < g_dev_count; i++) {
        if (g_devs[i].root == root) return &g_devs[i];
    } 


    return NULL;
}
#endif
// ===== END PATCH =====


/* Marked unused for now to keep build clean */
static __attribute__((unused)) void j_dev_set_active_idx(int idx)
{
    if (g_dev_count <= 0) {
        g_active_dev_idx = 0;
        return;
    }

    if (idx < 0) idx = 0;
    if (idx >= g_dev_count) idx = g_dev_count - 1;
    g_active_dev_idx = idx;
}



static void j_dev_switch_to(int idx)
{
    if (g_dev_count <= 0) return;

    /* clamp */
    if (idx < 0) idx = 0;
    if (idx >= g_dev_count) idx = g_dev_count - 1;

    lv_obj_t *root = g_devs[idx].root;
    if (!root) {
        LV_LOG_ERROR("j_dev_switch_to: root is NULL for idx=%d", idx);
        return;
    }

    g_active_dev_idx = idx;
    lv_scr_load(root);
}

static int j_dev_insert_before_diag(lv_obj_t *root, device_state_t *st)
{
    if (!root) return -1;
    if (g_dev_count <= 0) return -1;
    if (g_dev_count >= J_DEV_MAX) return -1;

    /* Rule: Diag is always last => insert at (g_dev_count - 1) */
    int insert_at = g_dev_count - 1;
    if (insert_at < 0) insert_at = 0;

    /* shift right */
    for (int i = g_dev_count; i > insert_at; i--) {
        g_devs[i] = g_devs[i - 1];
    }

    /* fill new slot */
    g_devs[insert_at].drv          = NULL;
    g_devs[insert_at].root         = root;
    g_devs[insert_at].stack_depth   = 0;
    g_devs[insert_at].overlay_depth = 0;
    g_devs[insert_at].st            = st;

        // ===== BEGIN PATCH: init per-device UI state =====
    g_devs[insert_at].brightness_percent = 30;
    g_devs[insert_at].speed_percent      = 20;
    g_devs[insert_at].dev_temp_view_mode = DEV_TEMP_VIEW_CURRENT;
    g_devs[insert_at].dev_pwr_view_mode  = DEV_PWR_VIEW_CURRENT;
    // ===== END PATCH =====



    g_dev_count++;

    /* keep active index sane */
    if (g_active_dev_idx >= insert_at) g_active_dev_idx++;

    return insert_at;
}



/* ============================================================
 *          FORWARD DECLARATIONS
 * ============================================================*/

static void device_screen_update_from_state(void);
static void update_compact_arcs_from_percent(void);

static void brightness_overlay_open(void);
static void brightness_overlay_close(void);
static void speed_overlay_open(void);
static void speed_overlay_close(void);

static void honeycomb_screen_update_from_state(void);
static void ui_refresh_all_screens(void);

static void honeycomb_center_event_cb(lv_event_t *e);
static void honeycomb_bottom_dev_container_event_cb(lv_event_t *e);

static void center_event_cb(lv_event_t *e);
static void screen_touch_event_cb(lv_event_t *e);
static void brightness_idle_event_cb(lv_event_t *e);
static void brightness_overlay_event_cb(lv_event_t *e);
static void speed_idle_event_cb(lv_event_t *e);
static void speed_overlay_event_cb(lv_event_t *e);
static void brightness_overlay_arc_event_cb(lv_event_t *e);
static void speed_overlay_arc_event_cb(lv_event_t *e);

static void room_container_event_cb(lv_event_t *e);
static void bottom_dev_container_event_cb(lv_event_t *e);

static lv_obj_t *ui_create_device_screen(void);
static void      diag_screen_update_from_state(void);
static lv_obj_t *ui_create_diag_screen(void);

static void device_screen_switch_to_next(void);
static void device_screen_switch_to_prev(void);

static bool point_in_arc_hitbox(const lv_point_t *p, int16_t arc_start, int16_t arc_end);



/* ============================================================
 *                     HELPERS
 * ============================================================*/

/* STEP 6: forward decls for helper in this section */
static inline j_dev_ctx_t *ui_active_dev_ctx(void);
static inline void ui_active_dev_set_overlay(bool open);
/* ===== ui_anim_overlay bindings ===== */
static void ui_anim_set_overlay(bool open)
{
    ui_active_dev_set_overlay(open);
}

// ===== BEGIN PATCH: set mode into active device state =====
static void ui_anim_set_mode(const char *mode_str)
{
    j_dev_ctx_t *d = ui_active_dev_ctx();
    if (d && d->st) d->st->mode = mode_str;
    else           g_current_device.mode = mode_str;
}
// ===== END PATCH =====


static void ui_anim_request_refresh(void)
{
    /* Safe: keep both screens in sync */
    ui_refresh_all_screens();
}


static void ui_refresh_all_screens(void)
{
    if (screen_device)    device_screen_update_from_state();
    if (screen_honeycomb) honeycomb_screen_update_from_state();
}


static bool ui_global_swipe_blocked(void)
{
    /* Primary rule: device state */
    j_dev_ctx_t *d = ui_active_dev_ctx();
    if (d) {
    if (d->overlay_depth > 0) return true;
    if (d->stack_depth > 0) return true;
}


    return false;
}


static inline j_dev_ctx_t *ui_active_dev_ctx(void)
{
    return j_active_dev(); /* from STEP 2 */
}

static inline void ui_active_dev_set_overlay(bool open)
{
    j_dev_ctx_t *d = ui_active_dev_ctx();
    if (!d) return;

    if (open) {
        if (d->overlay_depth < 255) d->overlay_depth++;
    } else {
        if (d->overlay_depth > 0) d->overlay_depth--;
    }
}




static void apply_hitbox_debug_to_panel(lv_obj_t *obj, bool enabled, lv_color_t color)
{
#if J_DEBUG_HITBOXES_GLOBAL
    if (!obj) return;
    if (enabled) {
        lv_obj_set_style_outline_width(obj, J_HITBOX_OUTLINE_WIDTH, 0);
        lv_obj_set_style_outline_color(obj, color, 0);
        lv_obj_set_style_outline_opa(obj, J_HITBOX_OPA_OUTLINE, 0);
        lv_obj_set_style_outline_pad(obj, 1, 0);
    }
#else
    (void)obj; (void)enabled; (void)color;
#endif
}


static bool point_in_arc_hitbox(const lv_point_t *p, int16_t arc_start, int16_t arc_end)
{
    if (!p) return false;
    if (g_arc_size <= 0 || g_screen_w <= 0 || g_screen_h <= 0) return false;

    float cx = g_screen_w * 0.5f;
    float cy = g_screen_h * 0.5f;

    float dx = (float)p->x - cx;
    float dy = (float)p->y - cy;

    float r = sqrtf(dx * dx + dy * dy);
    float R = (float)g_arc_size * 0.5f;

    float thickness = (float)g_arc_size * ARC_HIT_THICKNESS_RATIO;
    float r_inner   = R - thickness * 0.5f;
    float r_outer   = R + thickness * 0.5f;

    if (r < r_inner || r > r_outer) return false;

    float ang_math = atan2f(-dy, dx) * 180.0f / 3.14159265f;
    if (ang_math < 0.0f) ang_math += 360.0f;

    float ang_lv = 360.0f - ang_math;
    if (ang_lv >= 360.0f) ang_lv -= 360.0f;
    if (ang_lv < 0.0f)    ang_lv += 360.0f;

    int16_t start = arc_start;
    int16_t end   = arc_end;
    if (start < 0) start += 360;
    if (end   < 0) end   += 360;

    if (start <= end) return (ang_lv >= start && ang_lv <= end);
    return (ang_lv >= start || ang_lv <= end);
}

/* ============================================================
 *             SCREEN SWITCHING (SWIPE LEFT / RIGHT)
 * ============================================================*/

static void device_screen_switch_to_next(void)
{
    if (g_dev_count <= 0) return;

    int next = (g_active_dev_idx + 1) % g_dev_count;
    ESP_LOGI("UI", "Swipe LEFT -> switch to device %d", next);
    j_dev_switch_to(next);
}


static void device_screen_switch_to_prev(void)
{
    if (g_dev_count <= 0) return;

    int prev = (g_active_dev_idx - 1 + g_dev_count) % g_dev_count;
    ESP_LOGI("UI", "Swipe RIGHT -> switch to device %d", prev);
    j_dev_switch_to(prev);
}



/* ============================================================
 *                     EVENT CALLBACKS
 * ============================================================*/

static void center_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        uint32_t now = lv_tick_get();

        if (g_center_last_click_ms != 0 &&
            lv_tick_elaps(g_center_last_click_ms) < J_DOUBLE_TAP_MS) {

            g_center_last_click_ms = 0;
            LV_LOG_USER("Center double click: open animation overlay");
            ui_anim_overlay_open();
        } else {
            g_center_last_click_ms = now;
            static bool s_paused = false;
            s_paused = !s_paused;
            LV_LOG_USER("Center single click: toggle pause -> %d", s_paused);
            j_esn_send_pause(s_paused);

        }
    }
    // ===== BEGIN PATCH: toggle active device power =====
else if (code == LV_EVENT_LONG_PRESSED) {
    j_dev_ctx_t *d = ui_active_dev_ctx();
    device_state_t *st = (d && d->st) ? d->st : &g_current_device;

    st->is_on = !st->is_on;
    j_esn_send_power(st->is_on);

    LV_LOG_USER("Center long press: toggle power -> %d", st->is_on);
    ui_refresh_all_screens();
}
// ===== END PATCH =====

}

static void honeycomb_center_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        uint32_t now = lv_tick_get();

        if (g_honey_center_last_click_ms != 0 &&
            lv_tick_elaps(g_honey_center_last_click_ms) < J_DOUBLE_TAP_MS) {

            g_honey_center_last_click_ms = 0;
            LV_LOG_USER("HoneyComb center double click: open animation overlay");
            ui_anim_overlay_open();
        } else {
            g_honey_center_last_click_ms = now;
            static bool s_paused = false;
            s_paused = !s_paused;
            LV_LOG_USER("Center single click: toggle pause -> %d", s_paused);
            j_esn_send_pause(s_paused);

        }
    }
    // ===== BEGIN PATCH: toggle active device power (HoneyComb) =====
        else if (code == LV_EVENT_LONG_PRESSED) {
            j_dev_ctx_t *d = ui_active_dev_ctx();
            device_state_t *st = (d && d->st) ? d->st : &g_current_device;

            st->is_on = !st->is_on;
            j_esn_send_power(st->is_on);
            LV_LOG_USER("HoneyComb center long press: toggle power -> %d", st->is_on);
            ui_refresh_all_screens();
}
// ===== END PATCH =====

}

static void honeycomb_bottom_dev_container_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);

    lv_coord_t mid_x = (a.x1 + a.x2) / 2;

        // ===== BEGIN PATCH: per-device bottom modes =====
    j_dev_ctx_t *d = ui_active_dev_ctx();
    if (!d) return;

    if (p.x <= mid_x) {
        d->dev_temp_view_mode =
            (dev_temp_view_mode_t)((d->dev_temp_view_mode + 1) % DEV_TEMP_VIEW_COUNT);
        LV_LOG_USER("Dev temp view -> %d", (int)d->dev_temp_view_mode);
    } else {
        d->dev_pwr_view_mode =
            (dev_pwr_view_mode_t)((d->dev_pwr_view_mode + 1) % DEV_PWR_VIEW_COUNT);
        LV_LOG_USER("Dev power view -> %d", (int)d->dev_pwr_view_mode);
    }
    // ===== END PATCH =====


    ui_refresh_all_screens();
}


static void screen_touch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    /* Tuning knobs */
    const lv_coord_t SWIPE_MIN_PX    = 10;  /* сколько нужно пройти для активации */
    const lv_coord_t AXIS_LOCK_PX    = 5;   /* минимум проход когда фиксируем ось */
    const lv_coord_t VERTICAL_GUARD  = 100;  /* допускаемая диагональ */

    /* Gesture state (static, потому что это callback) */
    static lv_coord_t start_x = 0;
    static lv_coord_t start_y = 0;
    static bool touch_active = false;
    static bool axis_locked = false;
    static bool lock_horizontal = false;

    if (code == LV_EVENT_PRESSED) {
        touch_active = true;
        axis_locked = false;
        lock_horizontal = false;

        start_x = p.x;
        start_y = p.y;
        return;
    }

    if (code == LV_EVENT_PRESS_LOST) {
        touch_active = false;
        axis_locked = false;
        return;
    }

    if (!touch_active) return;

    if (code == LV_EVENT_PRESSING) {
        lv_coord_t dx = p.x - start_x;
        lv_coord_t dy = p.y - start_y;

        if (!axis_locked) {
            if (LV_ABS(dx) >= AXIS_LOCK_PX || LV_ABS(dy) >= AXIS_LOCK_PX) {
                axis_locked = true;
                lock_horizontal = (LV_ABS(dx) >= LV_ABS(dy));
            }
        }

        /* если жест явно горизонтальный, можно "забирать" его у дочерних виджетов */
        if (axis_locked && lock_horizontal) {
            lv_event_stop_bubbling(e);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED) {
        lv_coord_t dx = p.x - start_x;
        lv_coord_t dy = p.y - start_y;

        touch_active = false;

        /* блок глобального свайпа при оверлеях/подэкранах */
        if (ui_global_swipe_blocked()) return;

        /* если ось не залочилась, решаем по итоговому движению */
        if (!axis_locked) {
            axis_locked = true;
            lock_horizontal = (LV_ABS(dx) >= LV_ABS(dy));
        }

        /* вертикальный жест не считаем глобальным свайпом */
        if (!lock_horizontal) return;

        /* защита от диагонали: если слишком сильно по Y, не считать */
        if (LV_ABS(dy) > VERTICAL_GUARD) return;

        if (dx <= -SWIPE_MIN_PX) {
            device_screen_switch_to_next();
        } else if (dx >= SWIPE_MIN_PX) {
            device_screen_switch_to_prev();
        }

        return;
    }
}

    /* Дополнительно (не обязательно): можно залочить ось еще во время PRESSING
       но мы не подписаны на LV_EVENT_PRESSING на экранах сейчас.
       Если захочешь сделать еще жестче, добавим PRESSING и lock там. */



static void brightness_idle_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        if (point_in_arc_hitbox(&p, BRIGHTNESS_ARC_START, BRIGHTNESS_ARC_END)) {
            LV_LOG_USER("Brightness: long press IN arc hitbox -> open overlay");
            brightness_overlay_open();
        } else {
            LV_LOG_USER("Brightness: long press OUTSIDE arc hitbox -> ignore");
        }
    }
}

static void brightness_overlay_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_RELEASED ||
        code == LV_EVENT_PRESS_LOST ||
        code == LV_EVENT_CLICKED) {
        brightness_overlay_close();
    }
}

static void speed_idle_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        if (point_in_arc_hitbox(&p, SPEED_ARC_START, SPEED_ARC_END)) {
            LV_LOG_USER("Speed: long press IN arc hitbox -> open overlay");
            speed_overlay_open();
        } else {
            LV_LOG_USER("Speed: long press OUTSIDE arc hitbox -> ignore");
        }
    }
}

static void speed_overlay_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_RELEASED ||
        code == LV_EVENT_PRESS_LOST ||
        code == LV_EVENT_CLICKED) {
        speed_overlay_close();
    }
}

static void brightness_overlay_arc_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *arc = lv_event_get_target(e);
        int16_t v = lv_arc_get_value(arc);

        // ===== BEGIN PATCH: per-device brightness percent =====
        j_dev_ctx_t *d = ui_active_dev_ctx();
        if (d) d->brightness_percent = v;
        LV_LOG_USER("Brightness overlay value = %d%%", v);
        update_compact_arcs_from_percent();
        // ===== END PATCH =====

        // v = 0..100 (%). Переводим в 0..255 для лампы.
        uint16_t b = (uint16_t)((v * 255) / 100);
        if (b > 255) b = 255;
        j_esn_send_brightness_u8((uint8_t)b);
    }
}


static void speed_overlay_arc_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *arc = lv_event_get_target(e);
        int16_t v = lv_arc_get_value(arc);

        // ===== BEGIN PATCH: per-device speed percent =====
        j_dev_ctx_t *d = ui_active_dev_ctx();
        if (d) d->speed_percent = v;
        LV_LOG_USER("Speed overlay value = %d%%", v);
        update_compact_arcs_from_percent();
        // ===== END PATCH =====

        // v = 0..100 (%). Маппинг в 10..300 (%)
        uint16_t sp = 10 + (uint16_t)((v * (300 - 10)) / 100);
        if (sp < 10) sp = 10;
        if (sp > 300) sp = 300;
        j_esn_send_speed_pct(sp);
    }
}


static void room_container_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        g_room_view_mode = (room_view_mode_t)((g_room_view_mode + 1) % ROOM_VIEW_COUNT);
        LV_LOG_USER("Room view mode changed -> %d", (int)g_room_view_mode);
        ui_refresh_all_screens();

    }
}

static void bottom_dev_container_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);

    lv_coord_t mid_x = (a.x1 + a.x2) / 2;

        // ===== BEGIN PATCH: per-device bottom modes (HoneyComb) =====
    j_dev_ctx_t *d = ui_active_dev_ctx();
    if (!d) return;

    if (p.x <= mid_x) {
        d->dev_temp_view_mode =
            (dev_temp_view_mode_t)((d->dev_temp_view_mode + 1) % DEV_TEMP_VIEW_COUNT);
        LV_LOG_USER("HoneyComb dev temp view -> %d", (int)d->dev_temp_view_mode);
    } else {
        d->dev_pwr_view_mode =
            (dev_pwr_view_mode_t)((d->dev_pwr_view_mode + 1) % DEV_PWR_VIEW_COUNT);
        LV_LOG_USER("HoneyComb dev power view -> %d", (int)d->dev_pwr_view_mode);
    }
    // ===== END PATCH =====


    ui_refresh_all_screens();
}

/* ============================================================
 *               DEVICE SCREEN CREATION + UPDATE
 * ============================================================*/

static lv_obj_t *ui_create_device_screen(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        LV_LOG_ERROR("No default display found");
        return NULL;
    }

    lv_coord_t w = lv_disp_get_hor_res(disp);
    lv_coord_t h = lv_disp_get_ver_res(disp);

    g_screen_w    = w;
    g_screen_h    = h;
    g_screen_size = (w < h) ? w : h;

    screen_device = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_device, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_device, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(screen_device, 0, 0);
    lv_obj_set_style_outline_width(screen_device, 0, 0);
    lv_obj_set_size(screen_device, w, h);
    lv_obj_center(screen_device);

    lv_obj_set_style_bg_color(screen_device, J_COLOR_BG_MAIN, 0);
    lv_obj_set_style_bg_opa(screen_device, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen_device, J_COLOR_TEXT_MAIN, 0);

    lv_obj_add_event_cb(screen_device, screen_touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(screen_device, screen_touch_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(screen_device, screen_touch_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(screen_device, screen_touch_event_cb, LV_EVENT_PRESS_LOST, NULL);


    lv_coord_t arc_size = g_screen_size * ARC_SIZE_RATIO;
    g_arc_size = arc_size;

    brightness_arc = lv_arc_create(screen_device);
    lv_obj_set_size(brightness_arc, arc_size, arc_size);
    lv_obj_center(brightness_arc);
    lv_arc_set_bg_angles(brightness_arc, BRIGHTNESS_ARC_START, BRIGHTNESS_ARC_END);
    lv_obj_remove_style(brightness_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(brightness_arc, 8,  LV_PART_MAIN);
    lv_obj_set_style_arc_width(brightness_arc, 10, LV_PART_INDICATOR);
    lv_obj_move_background(brightness_arc);
    /*Правка для отключения автозаполнения шкалы и рассинхрона с хит зоной*/
    lv_obj_clear_flag(brightness_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(brightness_arc, LV_OBJ_FLAG_SCROLLABLE);


    speed_arc = lv_arc_create(screen_device);
    lv_obj_set_size(speed_arc, arc_size, arc_size);
    lv_obj_center(speed_arc);
    lv_arc_set_bg_angles(speed_arc, SPEED_ARC_START, SPEED_ARC_END);
    lv_obj_remove_style(speed_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(speed_arc, 8,  LV_PART_MAIN);
    lv_obj_set_style_arc_width(speed_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(speed_arc, J_COLOR_SPEED_ARC, LV_PART_INDICATOR);
    lv_obj_move_background(speed_arc);
    /* Правка для отключения автозаполнения шкалы и рассинхрона с хит зоной */
    lv_obj_clear_flag(speed_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(speed_arc, LV_OBJ_FLAG_SCROLLABLE);


#if J_DEBUG_HITBOXES_GLOBAL
    if (J_DEBUG_HITBOX_BRIGHTNESS) {
        lv_coord_t thickness_px = (lv_coord_t)(g_arc_size * ARC_HIT_THICKNESS_RATIO);
        lv_obj_t *dbg_b = lv_arc_create(screen_device);
        lv_obj_set_size(dbg_b, g_arc_size, g_arc_size);
        lv_obj_center(dbg_b);
        lv_arc_set_bg_angles(dbg_b, BRIGHTNESS_ARC_START, BRIGHTNESS_ARC_END);
        lv_arc_set_angles(dbg_b, BRIGHTNESS_ARC_START, BRIGHTNESS_ARC_END);
        lv_obj_remove_style(dbg_b, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(dbg_b, thickness_px, LV_PART_MAIN);
        lv_obj_set_style_arc_color(dbg_b, J_COLOR_HITBOX_BRIGHT, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(dbg_b, J_HITBOX_OPA_AREA, LV_PART_MAIN);
        lv_obj_clear_flag(dbg_b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(dbg_b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_move_background(dbg_b);
    }

    if (J_DEBUG_HITBOX_SPEED) {
        lv_coord_t thickness_px = (lv_coord_t)(g_arc_size * ARC_HIT_THICKNESS_RATIO);
        lv_obj_t *dbg_s = lv_arc_create(screen_device);
        lv_obj_set_size(dbg_s, g_arc_size, g_arc_size);
        lv_obj_center(dbg_s);
        lv_arc_set_bg_angles(dbg_s, SPEED_ARC_START, SPEED_ARC_END);
        lv_arc_set_angles(dbg_s, SPEED_ARC_START, SPEED_ARC_END);
        lv_obj_remove_style(dbg_s, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(dbg_s, thickness_px, LV_PART_MAIN);
        lv_obj_set_style_arc_color(dbg_s, J_COLOR_HITBOX_SPEED, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(dbg_s, J_HITBOX_OPA_AREA, LV_PART_MAIN);
        lv_obj_clear_flag(dbg_s, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(dbg_s, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_move_background(dbg_s);
    }
#endif

    update_compact_arcs_from_percent();

    center_container = lv_obj_create(screen_device);
    lv_obj_set_size(center_container,
                    g_screen_size * CENTER_W_RATIO,
                    g_screen_size * CENTER_H_RATIO);
    lv_obj_center(center_container);

    lv_obj_set_style_bg_opa(center_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(center_container, CENTER_BORDER_WIDTH, 0);
    lv_obj_set_style_border_color(center_container, J_COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_radius(center_container, g_screen_size * CENTER_RADIUS_RATIO, 0);
    lv_obj_set_style_pad_all(center_container, 8, 0);
    lv_obj_clear_flag(center_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(center_container, center_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(center_container, center_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_flag(center_container, LV_OBJ_FLAG_EVENT_BUBBLE);

    apply_hitbox_debug_to_panel(center_container,
                                J_DEBUG_HITBOX_CENTER,
                                J_COLOR_HITBOX_CENTER);

    label_name = lv_label_create(center_container);
    lv_label_set_text(label_name, g_current_device.name);
    jinny_apply_label_cfg(label_name, &J_LABEL_CFG_NAME);

    label_mode = lv_label_create(center_container);
    lv_label_set_text(label_mode, g_current_device.mode);
    jinny_apply_label_cfg(label_mode, &J_LABEL_CFG_MODE);

    top_room_container = lv_obj_create(screen_device);
    lv_obj_set_size(top_room_container,
                    g_screen_size * ROOM_W_RATIO,
                    g_screen_size * ROOM_H_RATIO);

    lv_obj_align(top_room_container,
                 LV_ALIGN_TOP_MID,
                 0,
                 g_screen_size * ROOM_Y_RATIO + ROOM_OFFSET_Y);

    lv_obj_set_style_radius(top_room_container, 12, 0);
    lv_obj_set_style_bg_opa(top_room_container, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(top_room_container, J_COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_width(top_room_container, 1, 0);
    lv_obj_set_style_border_color(top_room_container, J_COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_pad_all(top_room_container, 4, 0);
    lv_obj_clear_flag(top_room_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(top_room_container, LV_OBJ_FLAG_EVENT_BUBBLE);

    apply_hitbox_debug_to_panel(top_room_container,
                                J_DEBUG_HITBOX_TOP_ROOM,
                                J_COLOR_HITBOX_TOP);

    lv_obj_add_event_cb(top_room_container, room_container_event_cb, LV_EVENT_CLICKED, NULL);

    label_room = lv_label_create(top_room_container);
    jinny_apply_label_cfg(label_room, &J_LABEL_CFG_ROOM);

    bottom_dev_container = lv_obj_create(screen_device);
    lv_obj_set_size(bottom_dev_container,
                    g_screen_size * DEV_W_RATIO,
                    g_screen_size * DEV_H_RATIO);

    lv_obj_align(bottom_dev_container,
                 LV_ALIGN_BOTTOM_MID,
                 0,
                 -(g_screen_size * DEV_Y_RATIO) + DEV_OFFSET_Y);

    lv_obj_set_style_radius(bottom_dev_container, 12, 0);
    lv_obj_set_style_bg_opa(bottom_dev_container, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(bottom_dev_container, J_COLOR_PANEL_BG, 0);
    lv_obj_set_style_border_width(bottom_dev_container, 1, 0);
    lv_obj_set_style_border_color(bottom_dev_container, J_COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_pad_all(bottom_dev_container, 4, 0);
    lv_obj_clear_flag(bottom_dev_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(bottom_dev_container, LV_OBJ_FLAG_EVENT_BUBBLE);

    apply_hitbox_debug_to_panel(bottom_dev_container,
                                J_DEBUG_HITBOX_BOTTOM_DEV,
                                J_COLOR_HITBOX_BOTTOM);

    lv_obj_add_event_cb(bottom_dev_container, bottom_dev_container_event_cb, LV_EVENT_CLICKED, NULL);

    label_dev_temp = lv_label_create(bottom_dev_container);
    jinny_apply_label_cfg(label_dev_temp, &J_LABEL_CFG_DEV_TEMP);

    label_dev_power = lv_label_create(bottom_dev_container);
    jinny_apply_label_cfg(label_dev_power, &J_LABEL_CFG_DEV_POWER);

    brightness_touch_area = lv_obj_create(screen_device);
    lv_obj_set_size(brightness_touch_area,
                    g_screen_size * ARC_TOUCH_W_RATIO,
                    g_screen_size * ARC_TOUCH_H_RATIO);
    lv_obj_align(brightness_touch_area, LV_ALIGN_TOP_MID, 0,
                 g_screen_size * ARC_TOUCH_OFFSET);
    make_invisible_hit_area(brightness_touch_area);
    lv_obj_add_event_cb(brightness_touch_area, brightness_idle_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_flag(brightness_touch_area, LV_OBJ_FLAG_EVENT_BUBBLE);

    speed_touch_area = lv_obj_create(screen_device);
    lv_obj_set_size(speed_touch_area,
                    g_screen_size * ARC_TOUCH_W_RATIO,
                    g_screen_size * ARC_TOUCH_H_RATIO);
    lv_obj_align(speed_touch_area, LV_ALIGN_BOTTOM_MID, 0,
                 -g_screen_size * ARC_TOUCH_OFFSET);
    make_invisible_hit_area(speed_touch_area);
    lv_obj_add_event_cb(speed_touch_area, speed_idle_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_flag(speed_touch_area, LV_OBJ_FLAG_EVENT_BUBBLE);

    left_hint = lv_label_create(screen_device);
    lv_label_set_text(left_hint, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(left_hint, &lv_font_montserrat_24, 0);
    lv_obj_align(left_hint, LV_ALIGN_LEFT_MID,
                 g_screen_size * HINT_OFFSET_RATIO, 0);
    lv_obj_add_flag(left_hint, LV_OBJ_FLAG_EVENT_BUBBLE);

    right_hint = lv_label_create(screen_device);
    lv_label_set_text(right_hint, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(right_hint, &lv_font_montserrat_24, 0);
    lv_obj_align(right_hint, LV_ALIGN_RIGHT_MID,
                 -g_screen_size * HINT_OFFSET_RATIO, 0);
    lv_obj_add_flag(right_hint, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_move_foreground(center_container);
    lv_obj_move_foreground(top_room_container);
    lv_obj_move_foreground(bottom_dev_container);

    device_screen_update_from_state();

    return screen_device;
}

static void device_screen_update_from_state(void)
{
    // ===== BEGIN PATCH: screen-specific state via registry =====
    const device_state_t *st = &g_current_device;
        // ===== BEGIN PATCH: fetch per-device UI state for Lamp screen =====
    dev_temp_view_mode_t temp_mode = DEV_TEMP_VIEW_CURRENT;
    dev_pwr_view_mode_t  pwr_mode  = DEV_PWR_VIEW_CURRENT;

    j_dev_ctx_t *d_ui = j_dev_find_by_root(screen_device);
    if (d_ui) {
        temp_mode = d_ui->dev_temp_view_mode;
        pwr_mode  = d_ui->dev_pwr_view_mode;
    }
    // ===== END PATCH =====

    j_dev_ctx_t *d = j_dev_find_by_root(screen_device);
    if (d && d->st) st = d->st;
// ===== END PATCH =====


    lv_label_set_text(label_name, st->name);
    lv_label_set_text(label_mode, st->mode);

    lv_color_t mode_color = st->is_on
    ? lv_color_hex(J_MODE_COLOR_ACTIVE_HEX)
    : lv_color_hex(0x808080);


    lv_obj_set_style_text_color(label_mode, mode_color, 0);


    lv_color_t status_color;
    if (st->is_online && st->is_on) status_color = J_COLOR_STATUS_OK;
    else if (!st->is_online)        status_color = J_COLOR_STATUS_WARN;
    else                            status_color = J_COLOR_STATUS_OFF;

    lv_obj_set_style_text_color(label_name, status_color, 0);

    char buf[64];

    switch (g_room_view_mode) {
    case ROOM_VIEW_CURRENT:
    default:
        lv_snprintf(buf, sizeof(buf), "Room: %.1f°C\nRH: %.0f%%", st->room_temp, st->room_humidity);
        break;
    case ROOM_VIEW_LAST_10H:
        lv_snprintf(buf, sizeof(buf), "Room 10h\nT: %.1f°C\nRH: %.0f%%", st->room_temp, st->room_humidity);
        break;
    case ROOM_VIEW_LAST_24H:
        lv_snprintf(buf, sizeof(buf), "Room 24h\nT: %.1f°C\nRH: %.0f%%", st->room_temp, st->room_humidity);
        break;
    }
    lv_label_set_text(label_room, buf);

    char buf_temp[32];
    char buf_power[32];

    switch (temp_mode) {
    case DEV_TEMP_VIEW_CURRENT:
    default:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Dev: %.1f°C", st->device_temp);
        break;
    case DEV_TEMP_VIEW_MAX_TODAY:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Max today:\n%.1f°C", st->device_temp_max_today);
        break;
    case DEV_TEMP_VIEW_MAX_EVER:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Max ever:\n%.1f°C", st->device_temp_max_ever);
        break;
    }

    switch (pwr_mode) {
    case DEV_PWR_VIEW_CURRENT:
    default:
        lv_snprintf(buf_power, sizeof(buf_power), "%.1f W", st->power_w);
        break;
    case DEV_PWR_VIEW_MAX:
        lv_snprintf(buf_power, sizeof(buf_power), "Max:\n%.1f W", st->power_w_max);
        break;
    case DEV_PWR_VIEW_COST: {
        float p_kw = st->power_w / 1000.0f;
        float cost_per_hour = p_kw * st->energy_price_eur_per_kwh;
        lv_snprintf(buf_power, sizeof(buf_power), "Cost:\n€%.3f/h", cost_per_hour);
        break;
    }
    }

    lv_label_set_text(label_dev_temp,  buf_temp);
    lv_label_set_text(label_dev_power, buf_power);

    float ratio = 0.0f;
    if (st->lamp_theoretical_w > 0.0f) ratio = st->psu_max_w / st->lamp_theoretical_w;

    if (ratio < 0.7f)      lv_obj_set_style_arc_color(brightness_arc, J_COLOR_ARC_SAFE,   LV_PART_INDICATOR);
    else if (ratio < 0.95f)lv_obj_set_style_arc_color(brightness_arc, J_COLOR_ARC_WARN,   LV_PART_INDICATOR);
    else                   lv_obj_set_style_arc_color(brightness_arc, J_COLOR_ARC_DANGER, LV_PART_INDICATOR);

    update_compact_arcs_from_percent();
}

static void honeycomb_screen_update_from_state(void)
{
    // ===== BEGIN PATCH: screen-specific state via registry =====
    const device_state_t *st = &g_current_device;
        // ===== BEGIN PATCH: fetch per-device UI state for HoneyComb screen =====
    dev_temp_view_mode_t temp_mode = DEV_TEMP_VIEW_CURRENT;
    dev_pwr_view_mode_t  pwr_mode  = DEV_PWR_VIEW_CURRENT;

    j_dev_ctx_t *d_ui = j_dev_find_by_root(screen_honeycomb);
    if (d_ui) {
        temp_mode = d_ui->dev_temp_view_mode;
        pwr_mode  = d_ui->dev_pwr_view_mode;
    }
    // ===== END PATCH =====

    j_dev_ctx_t *d = j_dev_find_by_root(screen_honeycomb);
    if (d && d->st) st = d->st;
// ===== END PATCH =====


    /* Same logic as Lamp for colors */
    lv_color_t mode_color = st->is_on
        ? lv_color_hex(J_MODE_COLOR_ACTIVE_HEX)
        : lv_color_hex(0x808080);

    lv_color_t status_color;
    if (st->is_online && st->is_on) status_color = J_COLOR_STATUS_OK;
    else if (!st->is_online)        status_color = J_COLOR_STATUS_WARN;
    else                            status_color = J_COLOR_STATUS_OFF;

    ui_dev_honeycomb_set_center_text(st->name, st->mode);
    ui_dev_honeycomb_set_center_colors(status_color, mode_color);

    char buf_temp[32];
    char buf_power[32];

    switch (temp_mode) {
    case DEV_TEMP_VIEW_CURRENT:
    default:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Dev: %.1f°C", st->device_temp);
        break;
    case DEV_TEMP_VIEW_MAX_TODAY:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Max today:\n%.1f°C", st->device_temp_max_today);
        break;
    case DEV_TEMP_VIEW_MAX_EVER:
        lv_snprintf(buf_temp, sizeof(buf_temp), "Max ever:\n%.1f°C", st->device_temp_max_ever);
        break;
    }

    switch (pwr_mode) {
    case DEV_PWR_VIEW_CURRENT:
    default:
        lv_snprintf(buf_power, sizeof(buf_power), "%.1f W", st->power_w);
        break;
    case DEV_PWR_VIEW_MAX:
        lv_snprintf(buf_power, sizeof(buf_power), "Max:\n%.1f W", st->power_w_max);
        break;
    case DEV_PWR_VIEW_COST: {
        float p_kw = st->power_w / 1000.0f;
        float cost_per_hour = p_kw * st->energy_price_eur_per_kwh;
        lv_snprintf(buf_power, sizeof(buf_power), "Cost:\n€%.3f/h", cost_per_hour);
        break;
    }
    }

    ui_dev_honeycomb_set_bottom_text(buf_temp, buf_power);

    /* Brightness arc color same as Lamp */
    float ratio = 0.0f;
    if (st->lamp_theoretical_w > 0.0f) ratio = st->psu_max_w / st->lamp_theoretical_w;

    lv_color_t bright_col;
    if (ratio < 0.7f)       bright_col = J_COLOR_ARC_SAFE;
    else if (ratio < 0.95f) bright_col = J_COLOR_ARC_WARN;
    else                    bright_col = J_COLOR_ARC_DANGER;

    ui_dev_honeycomb_set_arc_colors(bright_col, J_COLOR_SPEED_ARC);

    // ===== BEGIN PATCH: HoneyComb arcs from its own ctx =====
    j_dev_ctx_t *d_hc = j_dev_find_by_root(screen_honeycomb);
    int16_t bp = d_hc ? d_hc->brightness_percent : 30;
    int16_t sp = d_hc ? d_hc->speed_percent      : 20;
    ui_dev_honeycomb_set_arc_percent(bp, sp);
// ===== END PATCH =====

}



/* ============================================================
 *      COMPACT ARCS UPDATE
 * ============================================================*/

static void update_compact_arcs_from_percent(void)
{
    /* Lamp arcs (lv_arc objects) */
    if (brightness_arc && speed_arc) {
        j_dev_ctx_t *d_lamp = j_dev_find_by_root(screen_device);

        int16_t bp = d_lamp ? d_lamp->brightness_percent : 30;
        int16_t sp = d_lamp ? d_lamp->speed_percent      : 20;

        if (bp < 0)   bp = 0;
        if (bp > 100) bp = 100;

        if (sp < 0)   sp = 0;
        if (sp > 100) sp = 100;


        int16_t b_start = BRIGHTNESS_ARC_START;
        int16_t b_end   = BRIGHTNESS_ARC_END;
        int16_t b_angle = b_start + (b_end - b_start) * bp / 100;

        int16_t s_start = SPEED_ARC_START;
        int16_t s_end   = SPEED_ARC_END;
        int16_t s_angle = s_start + (s_end - s_start) * sp / 100;

        lv_arc_set_angles(brightness_arc, b_start, b_angle);
        lv_arc_set_angles(speed_arc,      s_start, s_angle);
    }

    /* HoneyComb arcs (module) */
    if (screen_honeycomb) {
        j_dev_ctx_t *d_hc = j_dev_find_by_root(screen_honeycomb);

        int16_t bp = d_hc ? d_hc->brightness_percent : 30;
        int16_t sp = d_hc ? d_hc->speed_percent      : 20;

        if (bp < 0)   bp = 0;
        if (bp > 100) bp = 100;

        if (sp < 0)   sp = 0;
        if (sp > 100) sp = 100;


        ui_dev_honeycomb_set_arc_percent(bp, sp);
    }
}


/* ============================================================
 *        FULL-SCREEN BRIGHTNESS OVERLAY
 * ============================================================*/

static void brightness_overlay_open(void)
{
    if (brightness_overlay) return;
    ui_active_dev_set_overlay(true);

    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        ui_active_dev_set_overlay(false);
        return;
    }


    if (g_screen_size == 0 || g_screen_w == 0 || g_screen_h == 0) {
        lv_coord_t w = lv_disp_get_hor_res(disp);
        lv_coord_t h = lv_disp_get_ver_res(disp);
        g_screen_w    = w;
        g_screen_h    = h;
        g_screen_size = (w < h) ? w : h;
    }

        /* Create overlay on top layer (MUST exist) */
    lv_obj_t *top = lv_layer_top();
    if (!top) {
        LV_LOG_ERROR("brightness_overlay_open: lv_layer_top() is NULL");
        ui_active_dev_set_overlay(false);
        return;
    }

    brightness_overlay = lv_obj_create(top);
    if (!brightness_overlay) {
        LV_LOG_ERROR("brightness_overlay_open: lv_obj_create(top) returned NULL");
        ui_active_dev_set_overlay(false);
        return;
    }

    lv_obj_set_size(brightness_overlay, g_screen_w, g_screen_h);
    lv_obj_center(brightness_overlay);


    lv_obj_set_style_bg_opa(brightness_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(brightness_overlay, lv_color_hex(0x000000), 0);

    lv_obj_clear_flag(brightness_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(brightness_overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(brightness_overlay, brightness_overlay_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(brightness_overlay, brightness_overlay_event_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(brightness_overlay, brightness_overlay_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *arc = lv_arc_create(brightness_overlay);
    if (!arc) {
        LV_LOG_ERROR("brightness_overlay_open: lv_arc_create() returned NULL");
        ui_active_dev_set_overlay(false);
        j_lv_obj_del_safe(&brightness_overlay);
        return;
    }

    lv_obj_set_size(arc, g_screen_size * 0.90f, g_screen_size * 0.90f);

    lv_obj_center(arc);

    lv_arc_set_range(arc, 0, 100);
        j_dev_ctx_t *d = ui_active_dev_ctx();
    int16_t v0 = d ? d->brightness_percent : 30;
    lv_arc_set_value(arc, v0);
    lv_arc_set_bg_angles(arc, 0, 360);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, J_COLOR_ARC_SAFE, LV_PART_INDICATOR);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(arc, brightness_overlay_arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(arc, brightness_overlay_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(arc, brightness_overlay_event_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(arc, brightness_overlay_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(brightness_overlay);
    char buf[64];
    lv_snprintf(buf, sizeof(buf), "Brightness\n%.1f W", g_current_device.power_w);

    lv_label_set_text(label, buf);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, J_FONT_OVERLAY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_center(label);
}

static void brightness_overlay_close(void)
{
    ui_active_dev_set_overlay(false);
    j_lv_obj_del_safe(&brightness_overlay);
}


/* ============================================================
 *        FULL-SCREEN SPEED OVERLAY
 * ============================================================*/

static void speed_overlay_open(void)
{
    if (speed_overlay) return;
    ui_active_dev_set_overlay(true);

    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        ui_active_dev_set_overlay(false);
        return;
    }


    if (g_screen_size == 0 || g_screen_w == 0 || g_screen_h == 0) {
        lv_coord_t w = lv_disp_get_hor_res(disp);
        lv_coord_t h = lv_disp_get_ver_res(disp);
        g_screen_w    = w;
        g_screen_h    = h;
        g_screen_size = (w < h) ? w : h;
    }

        lv_obj_t *top = lv_layer_top();
    if (!top) {
        LV_LOG_ERROR("speed_overlay_open: lv_layer_top() is NULL");
        ui_active_dev_set_overlay(false);
        return;
    }

    speed_overlay = lv_obj_create(top);
    if (!speed_overlay) {
        LV_LOG_ERROR("speed_overlay_open: lv_obj_create(top) returned NULL");
        ui_active_dev_set_overlay(false);
        return;
    }

    lv_obj_set_size(speed_overlay, g_screen_w, g_screen_h);
    lv_obj_center(speed_overlay);


    lv_obj_set_style_bg_opa(speed_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(speed_overlay, lv_color_hex(0x000000), 0);

    lv_obj_clear_flag(speed_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(speed_overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(speed_overlay, speed_overlay_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(speed_overlay, speed_overlay_event_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(speed_overlay, speed_overlay_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *arc = lv_arc_create(speed_overlay);
    if (!arc) {
        LV_LOG_ERROR("speed_overlay_open: lv_arc_create() returned NULL");
        ui_active_dev_set_overlay(false);
        j_lv_obj_del_safe(&speed_overlay);
        return;
    }

    lv_obj_set_size(arc, g_screen_size * 0.90f, g_screen_size * 0.90f);

    lv_obj_center(arc);

    lv_arc_set_range(arc, 0, 100);
        j_dev_ctx_t *d = ui_active_dev_ctx();
    int16_t v0 = d ? d->speed_percent : 20;
    lv_arc_set_value(arc, v0);
    lv_arc_set_bg_angles(arc, 0, 360);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, J_COLOR_SPEED_ARC, LV_PART_INDICATOR);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(arc, speed_overlay_arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(arc, speed_overlay_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(arc, speed_overlay_event_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(arc, speed_overlay_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(speed_overlay);
    lv_label_set_text(label, "Animation\nspeed");

    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, J_FONT_OVERLAY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_center(label);
}

static void speed_overlay_close(void)
{
    ui_active_dev_set_overlay(false);
    j_lv_obj_del_safe(&speed_overlay);
}


/* ============================================================
 *        DIAG SCREEN
 * ============================================================*/

static void diag_screen_update_from_state(void)
{
    char buf[64];

    lv_snprintf(buf, sizeof(buf), "FPS: ~%d", 33);
    if (diag_label_fps) lv_label_set_text(diag_label_fps, buf);

    lv_snprintf(buf, sizeof(buf), "Uptime: 12 min");
    if (diag_label_batt) lv_label_set_text(diag_label_batt, buf);

    lv_snprintf(buf, sizeof(buf), "RTC: 21:37");
    if (diag_label_rtc) lv_label_set_text(diag_label_rtc, buf);

    lv_snprintf(buf, sizeof(buf), "Heap: 120k free");
    if (diag_label_heap) lv_label_set_text(diag_label_heap, buf);

    lv_snprintf(buf, sizeof(buf), "Net: %s", g_current_device.is_online ? "ONLINE" : "OFFLINE");
    if (diag_label_conn) lv_label_set_text(diag_label_conn, buf);

    lv_snprintf(buf, sizeof(buf), "WiFi: -65 dBm");
    if (diag_label_wifi_rssi) lv_label_set_text(diag_label_wifi_rssi, buf);

    lv_snprintf(buf, sizeof(buf), "IMU: QMI8658 OK");
    if (diag_label_imu) lv_label_set_text(diag_label_imu, buf);

    lv_snprintf(buf, sizeof(buf), "Motion: STABLE");
    if (diag_label_motion) lv_label_set_text(diag_label_motion, buf);

    if (diag_batt_inner && diag_batt_text) {
        int batt_percent = 73;
        if (batt_percent < 0) batt_percent = 0;
        if (batt_percent > 100) batt_percent = 100;

        lv_coord_t inner_w = (J_BATT_ICON_W - 4) * batt_percent / 100;
        if (inner_w < 0) inner_w = 0;
        lv_obj_set_width(diag_batt_inner, inner_w);

        lv_snprintf(buf, sizeof(buf), "%d%%", batt_percent);
        lv_label_set_text(diag_batt_text, buf);
    }
}

static lv_obj_t *ui_create_diag_screen(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        LV_LOG_ERROR("No default display for diag");
        return NULL;
    }

    lv_coord_t w = lv_disp_get_hor_res(disp);
    lv_coord_t h = lv_disp_get_ver_res(disp);

    screen_diag = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_diag, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_diag, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(screen_diag, 0, 0);
    lv_obj_set_style_outline_width(screen_diag, 0, 0);
    lv_obj_set_size(screen_diag, w, h);
    lv_obj_center(screen_diag);

    lv_obj_set_style_bg_color(screen_diag, J_COLOR_BG_DIAG, 0);
    lv_obj_set_style_bg_opa(screen_diag, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen_diag, J_COLOR_TEXT_MAIN, 0);

    diag_label_title = lv_label_create(screen_diag);
    lv_label_set_text(diag_label_title, "System data");
    lv_obj_set_style_text_font(diag_label_title, J_FONT_DIAG_TITLE, 0);
    lv_obj_set_style_text_align(diag_label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_title, LV_ALIGN_TOP_MID, 0, 12);

    const lv_coord_t col_offset = w / 4;
    const lv_coord_t base_y     = -6;
    const lv_coord_t step       = 20;

    diag_label_fps = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_fps, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_fps, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_fps, LV_ALIGN_CENTER, -col_offset, base_y - (step + step / 2));

    diag_label_batt = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_batt, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_batt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_batt, LV_ALIGN_CENTER, -col_offset, base_y - (step / 2));

    diag_label_rtc = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_rtc, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_rtc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_rtc, LV_ALIGN_CENTER, -col_offset, base_y + (step / 2));

    diag_label_heap = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_heap, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_heap, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_heap, LV_ALIGN_CENTER, -col_offset, base_y + (step + step / 2));

    diag_label_conn = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_conn, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_conn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_conn, LV_ALIGN_CENTER, col_offset, base_y - (step + step / 2));

    diag_label_wifi_rssi = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_wifi_rssi, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_wifi_rssi, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_wifi_rssi, LV_ALIGN_CENTER, col_offset, base_y - (step / 2));

    diag_label_imu = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_imu, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_imu, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_imu, LV_ALIGN_CENTER, col_offset, base_y + (step / 2));

    diag_label_motion = lv_label_create(screen_diag);
    lv_obj_set_style_text_font(diag_label_motion, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_align(diag_label_motion, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(diag_label_motion, LV_ALIGN_CENTER, col_offset, base_y + (step + step / 2));

    lv_obj_t *sep = lv_obj_create(screen_diag);
    lv_obj_set_size(sep, 2, (lv_coord_t)(h * 0.55f));
    lv_obj_align(sep, LV_ALIGN_CENTER, 0, base_y + 4);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(sep, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(sep, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(sep, J_COLOR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_outline_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 1, 0);

    diag_batt_container = lv_obj_create(screen_diag);
    lv_obj_set_size(diag_batt_container, J_BATT_CONT_W, J_BATT_CONT_H);
    lv_obj_align(diag_batt_container, LV_ALIGN_CENTER, J_BATT_POS_X, J_BATT_POS_Y);
    lv_obj_set_style_bg_opa(diag_batt_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(diag_batt_container, 0, 0);
    lv_obj_set_style_outline_width(diag_batt_container, 0, 0);
    lv_obj_set_style_pad_all(diag_batt_container, 0, 0);
    lv_obj_clear_flag(diag_batt_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *batt_box = lv_obj_create(diag_batt_container);
    lv_obj_set_size(batt_box, J_BATT_ICON_W, J_BATT_ICON_H);
    lv_obj_align(batt_box, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_opa(batt_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(batt_box, 2, 0);
    lv_obj_set_style_border_color(batt_box, J_COLOR_STATUS_OK, 0);
    lv_obj_set_style_radius(batt_box, 3, 0);

    diag_batt_inner = lv_obj_create(batt_box);
    lv_obj_set_size(diag_batt_inner, J_BATT_ICON_W - 4, J_BATT_ICON_H - 4);
    lv_obj_align(diag_batt_inner, LV_ALIGN_LEFT_MID, 1, 0);
    lv_obj_set_style_bg_opa(diag_batt_inner, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(diag_batt_inner, J_COLOR_STATUS_OK, 0);
    lv_obj_set_style_border_width(diag_batt_inner, 0, 0);
    lv_obj_set_style_radius(diag_batt_inner, 2, 0);

    lv_obj_t *batt_term = lv_obj_create(diag_batt_container);
    lv_obj_set_size(batt_term, 4, J_BATT_ICON_H - 6);
    lv_obj_align_to(batt_term, batt_box, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
    lv_obj_set_style_bg_opa(batt_term, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(batt_term, J_COLOR_STATUS_OK, 0);
    lv_obj_set_style_border_width(batt_term, 0, 0);
    lv_obj_set_style_radius(batt_term, 2, 0);

    diag_batt_text = lv_label_create(diag_batt_container);
    lv_obj_set_style_text_font(diag_batt_text, J_FONT_DIAG_TEXT, 0);
    lv_obj_set_style_text_color(diag_batt_text, J_COLOR_STATUS_OK, 0);
    lv_obj_set_style_text_align(diag_batt_text, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(diag_batt_text, "100%");
    lv_obj_align(diag_batt_text, LV_ALIGN_LEFT_MID, J_BATT_ICON_W + 12, 0);

    diag_screen_update_from_state();

    lv_obj_t *swipe_layer = lv_obj_create(screen_diag);
    lv_obj_set_size(swipe_layer, w, h);
    lv_obj_center(swipe_layer);
    make_invisible_hit_area(swipe_layer);
    lv_obj_add_flag(swipe_layer, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(swipe_layer, screen_touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(swipe_layer, screen_touch_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(swipe_layer, screen_touch_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(swipe_layer, screen_touch_event_cb, LV_EVENT_PRESS_LOST, NULL);


    return screen_diag;
}


static void ui_devices_init_registry(void)
{
    g_dev_count = 0;
        // ===== BEGIN PATCH: Lamp UI state =====
    g_devs[0].brightness_percent = 30;
    g_devs[0].speed_percent      = 20;
    g_devs[0].dev_temp_view_mode = DEV_TEMP_VIEW_CURRENT;
    g_devs[0].dev_pwr_view_mode  = DEV_PWR_VIEW_CURRENT;
    // ===== END PATCH =====

    g_active_dev_idx = 0;
        // ===== BEGIN PATCH: Diag UI state (unused, but keep deterministic) =====
    g_devs[1].brightness_percent = 30;
    g_devs[1].speed_percent      = 20;
    g_devs[1].dev_temp_view_mode = DEV_TEMP_VIEW_CURRENT;
    g_devs[1].dev_pwr_view_mode  = DEV_PWR_VIEW_CURRENT;
    // ===== END PATCH =====


    /* Lamp */
    g_devs[0].drv          = NULL;
    g_devs[0].root         = screen_device;
    g_devs[0].stack_depth   = 0;
    g_devs[0].overlay_depth = 0;
    g_devs[0].st           = &g_current_device;

    /* Diag (must stay last for insert-before-diag rule) */
    g_devs[1].drv          = NULL;
    g_devs[1].root         = screen_diag;
    g_devs[1].stack_depth   = 0;
    g_devs[1].overlay_depth = 0;
    g_devs[1].st           = &g_current_device;

    g_dev_count = 2;

    if (screen_honeycomb) {
        int idx = j_dev_insert_before_diag(screen_honeycomb, &g_honey_device);
        ESP_LOGI("UI", "HoneyComb inserted at idx=%d (Diag stays last idx=%d)", idx, g_dev_count - 1);
    } else {
        LV_LOG_ERROR("Failed to create HoneyComb screen");
    }
}



/* ============================================================
 *        LOW-LEVEL DRIVERS / TASKS
 * ============================================================*/

void Driver_Loop(void *parameter)
{
    (void)parameter;

    Wireless_Init();

    while (1) {
        QMI8658_Loop();
        PCF85063_Loop();
        BAT_Get_Volts();
        PWR_Loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

void Driver_Init(void)
{
    PWR_Init();
    BAT_Init();
    I2C_Init();
    EXIO_Init();
    Flash_Searching();
    PCF85063_Init();
    QMI8658_Init();

    xTaskCreatePinnedToCore(
        Driver_Loop,
        "Other Driver task",
        4096,
        NULL,
        3,
        NULL,
        0);
}

/* ============================================================
 *                         app_main
 * ============================================================*/

void app_main(void)
{
    Driver_Init();

    SD_Init();
    LCD_Init();
    Audio_Init();
    // MIC_Speech_Init();

    LVGL_Init();

    /* Clear any demo/system layers just once at boot */
    lv_obj_clean(lv_scr_act());
    lv_obj_clean(lv_layer_top());
    lv_obj_clean(lv_layer_sys());

    /* Create screens (lv_obj_create(NULL)) */
    screen_device    = ui_create_device_screen();
    screen_diag      = ui_create_diag_screen();
    
    ui_dev_honeycomb_cfg_t hc_cfg = {
    .swipe_cb           = screen_touch_event_cb,
    .center_cb          = honeycomb_center_event_cb,
    .bottom_cb          = honeycomb_bottom_dev_container_event_cb,
    .brightness_idle_cb = brightness_idle_event_cb,
    .speed_idle_cb      = speed_idle_event_cb,

    .font_name          = J_FONT_DEVICE_NAME,
    .font_mode          = J_FONT_DEVICE_NAME,
    .font_bottom        = J_FONT_BODY,
    .font_hint          = &lv_font_montserrat_24,

    .bg_color           = J_COLOR_BG_MAIN,
    .text_color         = J_COLOR_TEXT_MAIN,
    .panel_bg_color     = J_COLOR_PANEL_BG,
    .panel_border_color = J_COLOR_PANEL_BORDER,
    .speed_arc_color    = J_COLOR_SPEED_ARC,

    .title_text         = "Ambient",

    .bright_start       = BRIGHTNESS_ARC_START,
    .bright_end         = BRIGHTNESS_ARC_END,
    .speed_start        = SPEED_ARC_START,
    .speed_end          = SPEED_ARC_END,
};



screen_honeycomb = ui_dev_honeycomb_create(&hc_cfg);
if (screen_honeycomb) {
    ui_dev_honeycomb_set_center_text("HC OK", "after create");
    ui_dev_honeycomb_set_bottom_text("L OK", "R OK");
}



        /* Bind animation overlay module to current app context */
    ui_anim_overlay_bind_t anim_bind = {
        .p_screen_w       = &g_screen_w,
        .p_screen_h       = &g_screen_h,
        .p_screen_size    = &g_screen_size,
        .set_overlay      = ui_anim_set_overlay,
        .set_mode         = ui_anim_set_mode,
        .request_refresh  = ui_anim_request_refresh,
    };
    ui_anim_overlay_init(&anim_bind);


    /* Bind screens into device registry */
    ui_devices_init_registry();
    ui_refresh_all_screens();


    /* Load initial screen */
    if (screen_device) {
        j_dev_switch_to(0);                 /* uses registry root */
        lv_refr_now(lv_disp_get_default()); /* optional: force immediate redraw */
    } else {
        ESP_LOGE("UI", "screen_device is NULL, cannot start UI");
    }

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

