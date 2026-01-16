#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t j_espnow_link_start(void);
void      j_espnow_link_stop(void);
int j_esn_peer_register(uint16_t node_id, const uint8_t mac[6]);



/* ===== Existing CTRL API ===== */
esp_err_t j_esn_send_power(bool on);
esp_err_t j_esn_send_pause(bool paused);
esp_err_t j_esn_send_brightness_u8(uint8_t b);
esp_err_t j_esn_send_speed_pct(uint16_t pct);
esp_err_t j_esn_send_anim_id(uint16_t effect_id);

/* ===== FX list cache API (new) ===== */
bool     j_esn_fx_cache_valid(void);
uint16_t j_esn_fx_cache_count(void);
uint16_t j_esn_fx_cache_id_by_index(uint16_t idx);
const char *j_esn_fx_cache_name_by_index(uint16_t idx);

/* last known active effect_id from lamp ACK (0 if unknown yet) */
uint16_t j_esn_fx_last_effect_id(void);

/* Trigger sync (HELLO META -> CHUNKS). Safe to call multiple times. */
esp_err_t j_esn_fx_sync_start(void);

/* Optional: notify UI when cache updated */
typedef void (*j_esn_fx_updated_cb_t)(void *arg);
void j_esn_fx_set_updated_cb(j_esn_fx_updated_cb_t cb, void *arg);

/* ===== OTA (HELLO_OTA_INFO_RSP) ===== */

/* Start OTA session on lamp (lamp should later send HELLO_OTA_INFO_RSP with SSID/PASS) */
esp_err_t j_esn_send_ota_start(void);

/* Optional: notify UI when OTA info updated */
typedef void (*j_esn_ota_updated_cb_t)(void *arg);
void j_esn_ota_set_updated_cb(j_esn_ota_updated_cb_t cb, void *arg);

bool j_esn_ota_info_valid(void);

/* Copies SSID/PASS into provided buffers (always 0-terminated). */
void j_esn_ota_get_info(char *ssid, size_t ssid_sz,
                        char *pass, size_t pass_sz,
                        uint8_t *status, uint16_t *ttl_s);


#ifdef __cplusplus
}
#endif
