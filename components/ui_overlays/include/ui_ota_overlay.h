#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_coord_t *p_screen_w;
    lv_coord_t *p_screen_h;
    lv_coord_t *p_screen_size;

    void (*set_overlay)(bool open);
    void (*request_refresh)(void);

    /* Send OTA start command to lamp */
    void (*ota_send_start)(void);

    /* Get last OTA info received from lamp (HELLO_OTA_INFO_RSP) */
    void (*ota_get_info)(char *ssid, size_t ssid_sz,
                         char *pass, size_t pass_sz,
                         uint8_t *status, uint16_t *ttl_s);
} ui_ota_overlay_bind_t;

void ui_ota_overlay_init(const ui_ota_overlay_bind_t *bind);

void ui_ota_overlay_open(void);
void ui_ota_overlay_close(void);
bool ui_ota_overlay_is_open(void);

/* call when new HELLO_OTA_INFO_RSP arrived */
void ui_ota_overlay_refresh(void);

#ifdef __cplusplus
}
#endif
