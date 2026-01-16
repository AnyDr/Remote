#include "ui_ota_overlay.h"
#include <string.h>

/* =========================
 *   OTA OVERLAY CONFIG
 * =========================
 * Все значения ниже - “ручки” для тюнинга внешнего вида OTA-экрана.
 * Крутить можно безопасно: это только UI.
 */

/* ---------------------------------------------------------------------------
 * [A] ЛОГИКА / ЖЕСТЫ
 * ---------------------------------------------------------------------------
 */
#ifndef UI_OTA_DOUBLE_TAP_MS
#define UI_OTA_DOUBLE_TAP_MS 350 /*Увеличить -> проще успеть, уменьшить -> быстрее реагирует.*/
#endif

/* ---------------------------------------------------------------------------
 * [B] ФОНОВАЯ КАРТИНКА С SD (ДЕКОР, НЕ ВЛИЯЕТ НА ЛОГИКУ)
 * ---------------------------------------------------------------------------*/

#ifndef UI_OTA_BG_IMG_ENABLE
#define UI_OTA_BG_IMG_ENABLE 1 /* 1: пытаться загрузить картинку, 0: никогда не грузить.*/ 
#endif

#ifndef UI_OTA_BG_IMG_PATH
#define UI_OTA_BG_IMG_PATH "/sdcard/ui/ota/gear.png" /* путь на SD (VFS путь), туда кладёшь PNG.*/
#endif

#ifndef UI_OTA_BG_IMG_SIZE
#define UI_OTA_BG_IMG_SIZE 0 /* 0: не форсить размер (нативный), >0: принудительный размер (квадрат).*/
#endif

#ifndef UI_OTA_BG_IMG_OPA
#define UI_OTA_BG_IMG_OPA 220 /* прозрачность самой картинки (0..255), меньше -> “фонее”.*/
#endif

/* ---------------------------------------------------------------------------
 * [C] ФОН ОВЕРЛЕЯ: НОРМАЛЬНЫЙ И FALLBACK
 * --------------------------------------------------------------------------*/

#ifndef UI_OTA_BG_OPA
#define UI_OTA_BG_OPA ((lv_opa_t)200) /* прозрачность оверлея, когда картинка успешно прочиталась. */
#endif

#ifndef UI_OTA_FALLBACK_BG_COLOR
#define UI_OTA_FALLBACK_BG_COLOR lv_color_make(0, 0, 0) /* цвет фона, если картинка не загрузилась/не декодируется. */
#endif

#ifndef UI_OTA_FALLBACK_BG_OPA
#define UI_OTA_FALLBACK_BG_OPA ((lv_opa_t)255) /* непрозрачность fallback-фона (255 = полностью непрозрачный). */
#endif

/* ---------------------------------------------------------------------------
 * [D] ОКНО SSID/PASS (ПЛАШКА)
 * --------------------------------------------------------------------------*/
 
#ifndef UI_OTA_INFO_W_PCT
#define UI_OTA_INFO_W_PCT 50 /* ширина окна в % (100 = на весь экран; 60 = узкое по центру). */
#endif

#ifndef UI_OTA_INFO_H
#define UI_OTA_INFO_H 75 /* высота окна (px). */
#endif

#ifndef UI_OTA_INFO_Y
#define UI_OTA_INFO_Y 30 /* положение окна по Y (px от верха). Больше -> ближе к центру. */
#endif

#ifndef UI_OTA_INFO_RADIUS
#define UI_OTA_INFO_RADIUS 16 /* скругление углов окна (px). */
#endif

#ifndef UI_OTA_TOP_BG_OPA
#define UI_OTA_TOP_BG_OPA ((lv_opa_t)30) /* непрозрачность фона окна (0..255). 255 - непрозрачно вообще */
#endif

#ifndef UI_OTA_INFO_PAD_X
#define UI_OTA_INFO_PAD_X 18 /* отступ текста слева (px). */
#endif

#ifndef UI_OTA_INFO_PAD_Y
#define UI_OTA_INFO_PAD_Y 0 /* базовый отступ текста сверху (px). */
#endif

#ifndef UI_OTA_INFO_TEXT_Y
#define UI_OTA_INFO_TEXT_Y 0 /* ДОП сдвиг текста по Y внутри окна (px) + вниз, - вверх. */
#endif

#ifndef UI_OTA_PASS_DY
#define UI_OTA_PASS_DY 25 /* расстояние между строками SSID и PASS (px). */
#endif

#ifndef UI_OTA_TOP_FONT
#define UI_OTA_TOP_FONT (&lv_font_montserrat_20) /* шрифт строк SSID/PASS (меняет размер). */
#endif

#ifndef UI_OTA_TOP_TEXT_COLOR
#define UI_OTA_TOP_TEXT_COLOR lv_color_white() /* цвет текста SSID/PASS. */
#endif

#ifndef UI_OTA_INFO_BG_COLOR
#define UI_OTA_INFO_BG_COLOR lv_color_make(255,140,0) /* цвет плашки SSID/PASS */
#endif


/* ---------------------------------------------------------------------------
 * [E] КНОПКА OTA (КРУГЛАЯ)
 * --------------------------------------------------------------------------*/
 
#ifndef UI_OTA_BTN_DIAM
#define UI_OTA_BTN_DIAM 140 /* диаметр кнопки (px). */
#endif

#ifndef UI_OTA_BTN_COLOR
#define UI_OTA_BTN_COLOR lv_color_make(255, 140, 0) /* цвет кнопки (оранжевый по умолчанию). */
#endif

#ifndef UI_OTA_BTN_BG_OPA /* непрозрачность кнопки (обычно 255). */
#define UI_OTA_BTN_BG_OPA LV_OPA_COVER
#endif

#ifndef UI_OTA_BTN_TEXT_READY
#define UI_OTA_BTN_TEXT_READY "Start\nOTA" /* текст на кнопке в обычном состоянии (можно \n). */
#endif

#ifndef UI_OTA_BTN_TEXT_WAIT
#define UI_OTA_BTN_TEXT_WAIT "Waiting\n..." /* текст во время ожидания (можно \n) */
#endif

#ifndef UI_OTA_BTN_TEXT_COLOR
#define UI_OTA_BTN_TEXT_COLOR lv_color_white() /* цвет текста. */
#endif

#ifndef UI_OTA_BTN_FONT
#define UI_OTA_BTN_FONT (&lv_font_montserrat_28) /* шрифт текста кнопки (размер/вид). */
#endif

#ifndef UI_OTA_BTN_TEXT_PAD_X
#define UI_OTA_BTN_TEXT_PAD_X 16 /* “внутренняя ширина” для переносов: меньше -> раньше переносит строки. */
#endif

#ifndef UI_OTA_BTN_TEXT_LETTER_SPACE
#define UI_OTA_BTN_TEXT_LETTER_SPACE 0 /* межбуквенный интервал (0 обычно норм). */
#endif



static ui_ota_overlay_bind_t s_bind;
static bool s_inited = false;
static bool s_open = false;

static lv_obj_t *s_root = NULL;
static lv_obj_t *s_top = NULL;
static lv_obj_t *s_bg_img = NULL; /* decorative only */
static lv_obj_t *s_tap_catcher = NULL; /* invisible full-screen click layer */
static lv_obj_t *s_lbl_ssid = NULL;
static lv_obj_t *s_lbl_pass = NULL;
static lv_obj_t *s_btn = NULL;
static lv_obj_t *s_lbl_btn = NULL;

static uint32_t s_last_tap_ms = 0;
static uint8_t  s_tap_cnt = 0;

static void apply_info_to_labels(void)
{
    if (!s_bind.ota_get_info) return;

    char ssid[33] = {0};
    char pass[64] = {0};
    uint8_t st = 0;
    uint16_t ttl = 0;

    s_bind.ota_get_info(ssid, sizeof(ssid), pass, sizeof(pass), &st, &ttl);

    if (!s_lbl_ssid || !s_lbl_pass) return;

    if (st == 1 && ssid[0] && pass[0]) {
        lv_label_set_text_fmt(s_lbl_ssid, "SSID: %s", ssid);
        lv_label_set_text_fmt(s_lbl_pass, "PASS: %s", pass);
    } else {
        lv_label_set_text(s_lbl_ssid, "SSID: ----");
        lv_label_set_text(s_lbl_pass, "PASS: ----");
    }

    if (s_lbl_btn) lv_label_set_text(s_lbl_btn, UI_OTA_BTN_TEXT_READY);

}

static void close_overlay(void)
{
    if (!s_open) return;

    if (s_root) {
        s_bg_img = NULL;
        lv_obj_del(s_root);
        s_root = NULL;
    }
    s_top = NULL;
    s_tap_catcher = NULL;
    s_lbl_ssid = NULL;
    s_lbl_pass = NULL;
    s_btn = NULL;
    s_lbl_btn = NULL;
    


    s_open = false;
    s_tap_cnt = 0;
    s_last_tap_ms = 0;

    if (s_bind.set_overlay) s_bind.set_overlay(false);
    if (s_bind.request_refresh) s_bind.request_refresh();
}

static void root_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    /* double tap anywhere in center zone closes */
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_coord_t w = 0;
    lv_coord_t h = 0;

    if (s_bind.p_screen_w) w = *s_bind.p_screen_w;
    if (s_bind.p_screen_h) h = *s_bind.p_screen_h;

/* Fallback: if bind doesn't provide size, ask LVGL display */
if (w <= 0 || h <= 0) {
    lv_disp_t *d = lv_disp_get_default();
    if (d) {
        w = lv_disp_get_hor_res(d);
        h = lv_disp_get_ver_res(d);
    }
}
        if (w <= 0 || h <= 0) {
        return;
    }

    uint32_t now = lv_tick_get();
    if (now - s_last_tap_ms > UI_OTA_DOUBLE_TAP_MS) {
        s_tap_cnt = 0;
    }
    s_last_tap_ms = now;
    s_tap_cnt++;

    if (s_tap_cnt >= 2) {
        close_overlay();
    }
}

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    if (s_bind.ota_send_start) {
        /* optimistic UX: show waiting text */
        if (s_lbl_btn) lv_label_set_text(s_lbl_btn, UI_OTA_BTN_TEXT_WAIT);
        s_bind.ota_send_start();
    }
}

void ui_ota_overlay_init(const ui_ota_overlay_bind_t *bind)
{
    if (!bind) return;
    s_bind = *bind;
    s_inited = true;
}

bool ui_ota_overlay_is_open(void)
{
    return s_open;
}

void ui_ota_overlay_open(void)
{
    if (!s_inited || s_open) return;

    s_root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));

    /* Always force known background color (avoid theme default white). */
    lv_obj_set_style_bg_color(s_root, UI_OTA_FALLBACK_BG_COLOR, 0);

    /* Default to opaque fallback until we confirm image is readable/decodable. */
    lv_obj_set_style_bg_opa(s_root, UI_OTA_FALLBACK_BG_OPA, 0);
    /* Avoid any theme borders/outlines that show as white arcs on round LCD */
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_outline_width(s_root, 0, 0);
    lv_obj_set_style_shadow_width(s_root, 0, 0);

    /* Round corners (safe on round LCD) */
    lv_obj_set_style_radius(s_root, LV_RADIUS_CIRCLE, 0);


    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_root, root_event_cb, LV_EVENT_CLICKED, NULL);

    #if UI_OTA_BG_IMG_ENABLE
    /* Decorative background image (optional). Must not block input. */
    lv_img_header_t hdr;
    lv_res_t img_ok = lv_img_decoder_get_info(UI_OTA_BG_IMG_PATH, &hdr);

    if (img_ok == LV_RES_OK) {
        /* Image exists and decoder can read it -> allow normal overlay opacity. */
        lv_obj_set_style_bg_opa(s_root, UI_OTA_BG_OPA, 0);
        
        s_bg_img = lv_img_create(s_root);
        lv_img_set_src(s_bg_img, UI_OTA_BG_IMG_PATH);

        lv_obj_clear_flag(s_bg_img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(s_bg_img, LV_OBJ_FLAG_SCROLLABLE);

        if (UI_OTA_BG_IMG_SIZE > 0) {
            lv_obj_set_size(s_bg_img, UI_OTA_BG_IMG_SIZE, UI_OTA_BG_IMG_SIZE);
        }

        lv_obj_center(s_bg_img);
        lv_obj_set_style_opa(s_bg_img, (lv_opa_t)UI_OTA_BG_IMG_OPA, 0);
        lv_obj_move_background(s_bg_img);
    } else {
        /* Keep fallback opaque black. */
        s_bg_img = NULL;
    }
#else
    s_bg_img = NULL;
#endif


        /* SSID/PASS info window (full-width, movable) */
    s_top = lv_obj_create(s_root);
    lv_obj_clear_flag(s_top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_height(s_top, UI_OTA_INFO_H);
    lv_obj_set_width(s_top, LV_PCT(UI_OTA_INFO_W_PCT));
    lv_obj_align(s_top, LV_ALIGN_TOP_MID, 0, UI_OTA_INFO_Y);

    /* Window styling */
    lv_obj_set_style_radius(s_top, UI_OTA_INFO_RADIUS, 0);
    lv_obj_set_style_bg_opa(s_top, UI_OTA_TOP_BG_OPA, 0);
    lv_obj_set_style_bg_color(s_top, UI_OTA_INFO_BG_COLOR, 0);


    /* Kill borders/outlines that can show as white arcs on round LCD */
    lv_obj_set_style_border_width(s_top, 0, 0);
    lv_obj_set_style_outline_width(s_top, 0, 0);
    lv_obj_set_style_shadow_width(s_top, 0, 0);

    s_lbl_ssid = lv_label_create(s_top);
    s_lbl_pass = lv_label_create(s_top);

    lv_obj_align(s_lbl_ssid, LV_ALIGN_TOP_LEFT,
                 UI_OTA_INFO_PAD_X,
                 (lv_coord_t)(UI_OTA_INFO_PAD_Y + UI_OTA_INFO_TEXT_Y));
    lv_obj_set_style_text_font(s_lbl_ssid, UI_OTA_TOP_FONT, 0);
    lv_obj_set_style_text_color(s_lbl_ssid, UI_OTA_TOP_TEXT_COLOR, 0);
    lv_label_set_text(s_lbl_ssid, "SSID: ----");

    lv_obj_align(s_lbl_pass, LV_ALIGN_TOP_LEFT,
                 UI_OTA_INFO_PAD_X,
                 (lv_coord_t)(UI_OTA_INFO_PAD_Y + UI_OTA_INFO_TEXT_Y + UI_OTA_PASS_DY));
    lv_obj_set_style_text_font(s_lbl_pass, UI_OTA_TOP_FONT, 0);
    lv_obj_set_style_text_color(s_lbl_pass, UI_OTA_TOP_TEXT_COLOR, 0);
    lv_label_set_text(s_lbl_pass, "PASS: ----");

    /* Full-screen tap catcher:
     * catches taps everywhere for double-tap-to-close.
     * The OTA button is created after this, so it stays above and keeps its own clicks.
     */
    s_tap_catcher = lv_obj_create(s_root);
    lv_obj_set_size(s_tap_catcher, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_tap_catcher);

    lv_obj_set_style_bg_opa(s_tap_catcher, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_tap_catcher, 0, 0);
    lv_obj_set_style_outline_width(s_tap_catcher, 0, 0);
    lv_obj_set_style_shadow_width(s_tap_catcher, 0, 0);
    lv_obj_clear_flag(s_tap_catcher, LV_OBJ_FLAG_SCROLLABLE);

    /* Handle clicks here (button is above, so excluded automatically) */
    lv_obj_add_event_cb(s_tap_catcher, root_event_cb, LV_EVENT_CLICKED, NULL);





    /* Center button (round, orange, "OTA") */
    s_btn = lv_btn_create(s_root);
    lv_obj_set_size(s_btn, UI_OTA_BTN_DIAM, UI_OTA_BTN_DIAM);
    lv_obj_center(s_btn);
    lv_obj_add_event_cb(s_btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* Visuals */
    lv_obj_set_style_radius(s_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_btn, UI_OTA_BTN_BG_OPA, 0);
    lv_obj_set_style_bg_color(s_btn, UI_OTA_BTN_COLOR, 0);
    lv_obj_set_style_border_width(s_btn, 0, 0);
    lv_obj_set_style_outline_width(s_btn, 0, 0);
    lv_obj_set_style_shadow_width(s_btn, 0, 0);


    s_lbl_btn = lv_label_create(s_btn);
    lv_label_set_text(s_lbl_btn, UI_OTA_BTN_TEXT_READY);

    lv_obj_set_style_text_font(s_lbl_btn, UI_OTA_BTN_FONT, 0);
    lv_obj_set_style_text_color(s_lbl_btn, UI_OTA_BTN_TEXT_COLOR, 0);
    lv_obj_set_style_text_letter_space(s_lbl_btn, UI_OTA_BTN_TEXT_LETTER_SPACE, 0);

    /* Allow multi-line and keep it centered inside the round button */
    lv_label_set_long_mode(s_lbl_btn, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lbl_btn, (lv_coord_t)(UI_OTA_BTN_DIAM - UI_OTA_BTN_TEXT_PAD_X));
    lv_obj_set_style_text_align(s_lbl_btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_lbl_btn);




    s_open = true;
    if (s_bind.set_overlay) s_bind.set_overlay(true);

    apply_info_to_labels();
}

void ui_ota_overlay_close(void)
{
    close_overlay();

}

void ui_ota_overlay_refresh(void)
{
    if (!s_open) return;
    apply_info_to_labels();
}
