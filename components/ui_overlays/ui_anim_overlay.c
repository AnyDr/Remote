#include "ui_anim_overlay.h"

static lv_obj_t *s_overlay = NULL;

void ui_anim_overlay_open(void)
{
    /* stub: step 3.1 will move real code here */
    (void)s_overlay;
}

void ui_anim_overlay_close(void)
{
    /* stub */
}

bool ui_anim_overlay_is_open(void)
{
    return (s_overlay != NULL);
}
