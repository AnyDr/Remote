#pragma once

#include "lvgl.h"

/* ============================================================
 *  Remote_Fonts.h
 *  Minimal “known-good” font map for your UI code.
 *  Uses LVGL built-in Montserrat fonts.
 *
 *  If some sizes are not enabled in your LVGL config,
 *  switch them to the nearest available size.
 * ============================================================*/

/* Core UI fonts */
#define J_FONT_DEVICE_NAME   (&lv_font_montserrat_36)
#define J_FONT_SMALL_TITLE   (&lv_font_montserrat_18)
#define J_FONT_BODY          (&lv_font_montserrat_20)

/* Overlay / big titles */
#define J_FONT_OVERLAY       (&lv_font_montserrat_28)
#define J_FONT_DIAG_TITLE    (&lv_font_montserrat_24)
#define J_FONT_DIAG_TEXT     (&lv_font_montserrat_16)

/* Animation wheel font profiles */
#define J_WHEEL_FONT_14      (&lv_font_montserrat_14)
#define J_WHEEL_FONT_18      (&lv_font_montserrat_18)
#define J_WHEEL_FONT_22      (&lv_font_montserrat_22)
#define J_WHEEL_FONT_28      (&lv_font_montserrat_28)

/* Some LVGL builds don’t ship 34; 32 is usually available */
#define J_WHEEL_FONT_34      (&lv_font_montserrat_32)
#define J_WHEEL_FONT_40      (&lv_font_montserrat_40)
