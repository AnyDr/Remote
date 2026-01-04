#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t j_espnow_link_start(void);

/* API для UI */
esp_err_t j_esn_send_power(bool on);
esp_err_t j_esn_send_pause(bool paused);
esp_err_t j_esn_send_brightness_u8(uint8_t b);
esp_err_t j_esn_send_speed_pct(uint16_t pct);
esp_err_t j_esn_send_anim_id(uint16_t effect_id);

#ifdef __cplusplus
}
#endif
