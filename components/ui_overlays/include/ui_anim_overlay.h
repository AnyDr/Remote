#pragma once

#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_anim_overlay_open(void);
void ui_anim_overlay_close(void);
bool ui_anim_overlay_is_open(void);

#ifdef __cplusplus
}
#endif
