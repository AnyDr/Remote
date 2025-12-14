#pragma once

/* ============================================================
 *  Remote_UI_Layout.h
 *  Layout constants used by main.c.
 *
 *  All ratios are relative to g_screen_size (min(w,h)).
 * ============================================================*/

/* Big arcs diameter relative to screen */
#define ARC_SIZE_RATIO            (0.92f)   // диаметр дуг относительно размера экрана

/* Center panel (device name + mode) */
#define CENTER_W_RATIO            (0.70f)   // ширина центрального окна (доля экрана)
#define CENTER_H_RATIO            (0.36f)   // высота центрального окна (доля экрана)
#define CENTER_BORDER_WIDTH       (2)       // толщина рамки центрального окна (px)
#define CENTER_RADIUS_RATIO       (0.05f)   // радиус скругления углов (доля от размера окна)

/* Top “Room” window */
#define ROOM_W_RATIO              (0.45f)   // ширина верхнего окна (доля экрана)
#define ROOM_H_RATIO              (0.16f)   // высота верхнего окна (доля экрана)
#define ROOM_Y_RATIO              (0.13f)   // базовое смещение вниз от верхнего края (доля экрана)
#define ROOM_OFFSET_Y             (8)       // дополнительная подстройка по Y (px), + вниз

/* Bottom “Device” window */
#define DEV_W_RATIO               (0.45f)   // ширина нижнего окна (доля экрана)
#define DEV_H_RATIO               (0.16f)   // высота нижнего окна (доля экрана)
#define DEV_Y_RATIO               (0.13f)   // базовое смещение вверх от нижнего края (доля экрана)
#define DEV_OFFSET_Y              (-8)      // дополнительная подстройка по Y (px), + вверх / - вниз

/* Long-press touch areas for arcs (invisible rectangles) */
#define ARC_TOUCH_W_RATIO         (0.92f)   // ширина зоны тача дуг (доля экрана)
#define ARC_TOUCH_H_RATIO         (0.28f)   // высота зоны тача дуг (доля экрана)
#define ARC_TOUCH_OFFSET          (0.06f)   // смещение зон тача от центра дуг (доля экрана)

/* Left/right arrow hints position */
#define HINT_OFFSET_RATIO         (0.03f)   // смещение стрелок-подсказок от края (доля экрана)
