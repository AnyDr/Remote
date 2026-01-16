#include "j_espnow_link.h"
#include "j_espnow_proto.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "nvs.h"
#include "nvs_flash.h"


static const char *TAG = "J_ESN_R";

static uint32_t s_seq = 1;
static uint8_t  s_peer_mac[6] = {0};
static bool     s_peer_ok = false;

/* =========================
 * FX LIST CACHE (RAM + NVS)
 * ========================= */

#ifndef J_ESN_FX_CACHE_MAX
#define J_ESN_FX_CACHE_MAX 256
#endif

typedef struct __attribute__((packed)) {
    uint16_t count;
    uint32_t crc32;
} j_esn_fx_meta_nvs_t;

#define J_ESN_PEER_MAX  4

typedef struct {
    bool     in_use;
    uint8_t  mac[6];
    uint16_t node_id;

    /* FX cache (per device) */
    bool     fx_valid;
    uint16_t fx_count;
    uint32_t fx_crc32;
    j_esn_fx_entry_t fx_entries[J_ESN_FX_CACHE_MAX];

    /* last ACK (per device) */
    uint16_t last_effect_id;
    uint32_t last_state_seq;
        /* OTA info (HELLO_OTA_INFO_RSP) */
    uint8_t  ota_status;
    uint16_t ota_ttl_s;
    char     ota_ssid[J_ESN_OTA_SSID_MAX + 1];
    char     ota_pass[J_ESN_OTA_PASS_MAX + 1];

} j_esn_peer_t;

static j_esn_peer_t s_peers[J_ESN_PEER_MAX];
static int          s_peer_count = 0;

static inline j_esn_peer_t *peer0(void)
{
    return &s_peers[0];
}


/* sync state */
static bool     s_fx_sync_in_progress = false;
static uint16_t s_fx_sync_next_index  = 0;
static uint16_t s_fx_sync_total       = 0;
static uint32_t s_fx_sync_crc32       = 0;

/* update callback */
static j_esn_fx_updated_cb_t s_fx_updated_cb = NULL;
static void *s_fx_updated_arg = NULL;
static j_esn_ota_updated_cb_t s_ota_updated_cb = NULL;
static void *s_ota_updated_arg = NULL;

static void ota_notify_updated(void)
{
    if (s_ota_updated_cb) s_ota_updated_cb(s_ota_updated_arg);
}


static void fx_notify_updated(void)
{
    if (s_fx_updated_cb) s_fx_updated_cb(s_fx_updated_arg);
}

static void fx_cache_clear(void)
{
    j_esn_peer_t *p = peer0();
    p->fx_valid = false;
    p->fx_count = 0;
    p->fx_crc32 = 0;
    memset(p->fx_entries, 0, sizeof(p->fx_entries));
}


static esp_err_t fx_cache_load_nvs(void)
{
    nvs_handle_t nvh;
    esp_err_t err = nvs_open("j_esn_fx", NVS_READONLY, &nvh);
    if (err != ESP_OK) return err;

    j_esn_fx_meta_nvs_t meta = {0};
    size_t sz = sizeof(meta);
    err = nvs_get_blob(nvh, "meta", &meta, &sz);
    if (err != ESP_OK || sz != sizeof(meta)) {
        nvs_close(nvh);
        return ESP_FAIL;
    }

    if (meta.count == 0 || meta.count > J_ESN_FX_CACHE_MAX) {
        nvs_close(nvh);
        return ESP_FAIL;
    }

    size_t list_sz = 0;
    err = nvs_get_blob(nvh, "list", NULL, &list_sz);
    if (err != ESP_OK || list_sz != (size_t)meta.count * sizeof(j_esn_fx_entry_t)) {
        nvs_close(nvh);
        return ESP_FAIL;
    }

    j_esn_peer_t *p = peer0();
    memset(p->fx_entries, 0, sizeof(p->fx_entries));
    err = nvs_get_blob(nvh, "list", p->fx_entries, &list_sz);

    nvs_close(nvh);
    if (err != ESP_OK) return err;

    /* sanitize names */
    for (uint16_t i = 0; i < meta.count; i++) {
        p->fx_entries[i].name[J_ESN_FX_NAME_MAX - 1] = '\0';
    }

    p->fx_count = meta.count;
    p->fx_crc32 = meta.crc32;
    p->fx_valid = true;

    ESP_LOGI(TAG, "FX cache loaded: count=%u crc=0x%08X",
            (unsigned)p->fx_count, (unsigned)p->fx_crc32);


    return ESP_OK;
}

static esp_err_t fx_cache_save_nvs(void)
{
    j_esn_peer_t *p = peer0();
    if (!p->fx_valid || p->fx_count == 0) return ESP_ERR_INVALID_STATE;


    nvs_handle_t nvh;
    esp_err_t err = nvs_open("j_esn_fx", NVS_READWRITE, &nvh);
    if (err != ESP_OK) return err;

    j_esn_fx_meta_nvs_t meta = {
    .count = p->fx_count,
    .crc32 = p->fx_crc32,
    };


    err = nvs_set_blob(nvh, "meta", &meta, sizeof(meta));
    if (err == ESP_OK) {
        err = nvs_set_blob(nvh, "list", p->fx_entries, (size_t)p->fx_count * sizeof(j_esn_fx_entry_t));

    }
    if (err == ESP_OK) {
        err = nvs_commit(nvh);
    }
    nvs_close(nvh);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "FX cache saved: count=%u crc=0x%08X",
                (unsigned)p->fx_count, (unsigned)p->fx_crc32);


    }
    return err;
}

/* ---- HELLO send helpers ---- */

static esp_err_t send_hello_meta_req(void)
{
    if (!s_peer_ok) return ESP_ERR_INVALID_STATE;

    j_esn_fx_meta_req_t m = {0};
    m.h.magic    = J_ESN_MAGIC;
    m.h.ver      = J_ESN_VER;
    m.h.type     = J_ESN_MSG_HELLO;
    m.h.src_node = (uint16_t)CONFIG_J_NODE_ID;
    m.h.dst_node = 0xFFFF;
    m.h.seq      = s_seq++;
    m.hello_cmd  = J_ESN_HELLO_FX_META_REQ;

    return esp_now_send(s_peer_mac, (const uint8_t*)&m, sizeof(m));
}

static esp_err_t send_hello_chunk_req(uint16_t start_index)
{
    if (!s_peer_ok) return ESP_ERR_INVALID_STATE;

    j_esn_fx_chunk_req_t m = {0};
    m.h.magic      = J_ESN_MAGIC;
    m.h.ver        = J_ESN_VER;
    m.h.type       = J_ESN_MSG_HELLO;
    m.h.src_node   = (uint16_t)CONFIG_J_NODE_ID;
    m.h.dst_node   = 0xFFFF;
    m.h.seq        = s_seq++;
    m.hello_cmd    = J_ESN_HELLO_FX_CHUNK_REQ;
    m.start_index  = start_index;

    return esp_now_send(s_peer_mac, (const uint8_t*)&m, sizeof(m));
}


static bool parse_hex_byte(const char *s, uint8_t *out)
{
    if (!isxdigit((unsigned char)s[0]) || !isxdigit((unsigned char)s[1])) return false;
    char tmp[3] = { s[0], s[1], 0 };
    *out = (uint8_t)strtoul(tmp, NULL, 16);
    return true;
}

static bool parse_mac(const char *str, uint8_t mac[6])
{
    if (!str || strlen(str) != 17) return false;
    for (int i = 0; i < 6; i++) {
        const int p = i * 3;
        if (!parse_hex_byte(&str[p], &mac[i])) return false;
        if (i < 5 && str[p + 2] != ':') return false;
    }
    return true;
}

/* IDF 5.5: send-cb принимает wifi_tx_info_t* */
static void on_sent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info;
    if (status != ESP_NOW_SEND_SUCCESS) {
        ESP_LOGW(TAG, "send failed");
    }
}

static int peer_find_by_mac(const uint8_t mac[6])
{
    for (int i = 0; i < J_ESN_PEER_MAX; i++) {
        if (s_peers[i].in_use && memcmp(s_peers[i].mac, mac, 6) == 0) return i;
    }
    return -1;
}


static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!info || !data || len < (int)sizeof(j_esn_hdr_t)) return;

    const j_esn_hdr_t *h = (const j_esn_hdr_t*)data;
    if (h->magic != J_ESN_MAGIC || h->ver != J_ESN_VER) return;

    if (h->type == J_ESN_MSG_ACK) {
        if (len < (int)sizeof(j_esn_ack_t)) return;

        const j_esn_ack_t *a = (const j_esn_ack_t*)data;
        j_esn_peer_t *p = peer0();
        p->last_effect_id = a->effect_id;
        p->last_state_seq = a->state_seq;


        ESP_LOGI(TAG, "ACK seq=%u effect=%u bright=%u paused=%u speed=%u",
                 (unsigned)a->ack_seq,
                 (unsigned)a->effect_id,
                 (unsigned)a->brightness,
                 (unsigned)a->paused,
                 (unsigned)a->speed_pct);
        return;
    }

    if (h->type == J_ESN_MSG_HELLO) {
        /* at least hdr + hello_cmd */
        if (len < (int)(sizeof(j_esn_hdr_t) + 1)) return;

        const uint8_t *p = (const uint8_t*)data;
        uint8_t hello_cmd = p[sizeof(j_esn_hdr_t)];

        if (hello_cmd == J_ESN_HELLO_OTA_INFO_RSP) {
            if (len < (int)sizeof(j_esn_ota_info_rsp_t)) return;

            const j_esn_ota_info_rsp_t *rsp = (const j_esn_ota_info_rsp_t*)data;
            j_esn_peer_t *pp = peer0();

            pp->ota_status = rsp->ota_status;
            pp->ota_ttl_s  = rsp->ttl_s;

            memcpy(pp->ota_ssid, rsp->ssid, sizeof(pp->ota_ssid));
            pp->ota_ssid[sizeof(pp->ota_ssid) - 1] = '\0';

            memcpy(pp->ota_pass, rsp->pass, sizeof(pp->ota_pass));
            pp->ota_pass[sizeof(pp->ota_pass) - 1] = '\0';

            ESP_LOGI(TAG, "OTA INFO: status=%u ttl=%us ssid='%s'",
                     (unsigned)pp->ota_status,
                     (unsigned)pp->ota_ttl_s,
                     pp->ota_ssid);

            ota_notify_updated();
            return;
        }


        if (hello_cmd == J_ESN_HELLO_FX_META_RSP) {
            if (len < (int)sizeof(j_esn_fx_meta_rsp_t)) return;
            const j_esn_fx_meta_rsp_t *rsp = (const j_esn_fx_meta_rsp_t*)data;

            ESP_LOGI(TAG, "FX META: count=%u crc=0x%08X",
                     (unsigned)rsp->fx_count, (unsigned)rsp->fx_crc32);

            if (rsp->fx_count == 0 || rsp->fx_count > J_ESN_FX_CACHE_MAX) {
                ESP_LOGW(TAG, "FX META: unsupported count=%u (max=%u)",
                         (unsigned)rsp->fx_count, (unsigned)J_ESN_FX_CACHE_MAX);
                return;
            }

            /* If cache matches, nothing to do */
            j_esn_peer_t *p = peer0();
            if (p->fx_valid && p->fx_count == rsp->fx_count && p->fx_crc32 == rsp->fx_crc32) {

                ESP_LOGI(TAG, "FX META: cache already up-to-date");
                s_fx_sync_in_progress = false;
                return;
            }

            /* Start sync */
            s_fx_sync_in_progress = true;
            s_fx_sync_next_index  = 0;
            s_fx_sync_total       = rsp->fx_count;
            s_fx_sync_crc32       = rsp->fx_crc32;

            /* clear target buffer (but keep old cache valid until done) */
            memset(p->fx_entries, 0, sizeof(p->fx_entries));


            (void)send_hello_chunk_req(0);
            return;
        }

        if (hello_cmd == J_ESN_HELLO_FX_CHUNK_RSP) {
            if (len < (int)(sizeof(j_esn_hdr_t) + 1 + 1 + 2 + 4)) return;
            const j_esn_fx_chunk_rsp_t *rsp = (const j_esn_fx_chunk_rsp_t*)data;

            if (!s_fx_sync_in_progress) {
                ESP_LOGW(TAG, "FX CHUNK: ignored (sync not in progress)");
                return;
            }
            if (rsp->fx_crc32 != s_fx_sync_crc32) {
                ESP_LOGW(TAG, "FX CHUNK: crc mismatch rsp=0x%08X exp=0x%08X",
                         (unsigned)rsp->fx_crc32, (unsigned)s_fx_sync_crc32);
                return;
            }
            if (rsp->start_index != s_fx_sync_next_index) {
                ESP_LOGW(TAG, "FX CHUNK: unexpected start=%u expected=%u",
                         (unsigned)rsp->start_index, (unsigned)s_fx_sync_next_index);
                return;
            }
            if (rsp->count == 0 || rsp->count > J_ESN_FX_CHUNK_MAX) {
                ESP_LOGW(TAG, "FX CHUNK: bad count=%u", (unsigned)rsp->count);
                return;
            }

            uint16_t n = rsp->count;
            if ((uint16_t)(s_fx_sync_next_index + n) > s_fx_sync_total) {
                n = (uint16_t)(s_fx_sync_total - s_fx_sync_next_index);
            }

            for (uint16_t i = 0; i < n; i++) {
                uint16_t dst = (uint16_t)(s_fx_sync_next_index + i);
                j_esn_peer_t *p = peer0();
                p->fx_entries[dst] = rsp->entries[i];
                p->fx_entries[dst].name[J_ESN_FX_NAME_MAX - 1] = '\0';

            }

            s_fx_sync_next_index = (uint16_t)(s_fx_sync_next_index + n);

            ESP_LOGI(TAG, "FX CHUNK: got %u items, progress %u/%u",
                     (unsigned)n,
                     (unsigned)s_fx_sync_next_index,
                     (unsigned)s_fx_sync_total);

            if (s_fx_sync_next_index < s_fx_sync_total) {
                (void)send_hello_chunk_req(s_fx_sync_next_index);
                return;
            }

            /* Done: commit new cache */
            j_esn_peer_t *p = peer0();
            p->fx_count = s_fx_sync_total;
            p->fx_crc32 = s_fx_sync_crc32;
            p->fx_valid = true;

            s_fx_sync_in_progress = false;

            (void)fx_cache_save_nvs();
            fx_notify_updated();

            ESP_LOGI(TAG, "FX SYNC DONE: count=%u crc=0x%08X",
         (          unsigned)p->fx_count, (unsigned)p->fx_crc32);

            return;
        }

        return;
    }
}

int j_esn_peer_register(uint16_t node_id, const uint8_t mac[6])
{
    if (!mac) return -1;

    int existing = peer_find_by_mac(mac);
    if (existing >= 0) {
        s_peers[existing].node_id = node_id;
        return existing;
    }

    for (int i = 0; i < J_ESN_PEER_MAX; i++) {
        if (!s_peers[i].in_use) {
            memset(&s_peers[i], 0, sizeof(s_peers[i]));
            s_peers[i].in_use = true;
            memcpy(s_peers[i].mac, mac, 6);
            s_peers[i].node_id = node_id;
            if (i >= s_peer_count) s_peer_count = i + 1;
            return i;
        }
    }
    return -1;
}



esp_err_t j_espnow_link_start(void)
{
    if (!s_peer_ok) {
        if (strlen(CONFIG_J_ESPNOW_PEER_MAC) == 0) {
            ESP_LOGW(TAG, "Peer MAC empty -> ESPNOW start without peer");
            ESP_LOGI(TAG, "CONFIG_J_ESPNOW_PEER_MAC='%s'", CONFIG_J_ESPNOW_PEER_MAC);

        } else {
            if (!parse_mac(CONFIG_J_ESPNOW_PEER_MAC, s_peer_mac)) {
                ESP_LOGE(TAG, "Bad peer MAC format: '%s'", CONFIG_J_ESPNOW_PEER_MAC);
                return ESP_ERR_INVALID_ARG;
            }
            s_peer_ok = true;
        }
    }

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_sent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    /* Load FX cache from NVS if present (NVS must be initialized before this call) */
    fx_cache_clear();
    (void)fx_cache_load_nvs();


    if (s_peer_ok) {
        esp_now_peer_info_t p = {0};
        memcpy(p.peer_addr, s_peer_mac, 6);
        p.ifidx = WIFI_IF_STA;
        p.channel = 0;     // текущий канал Wi-Fi
        p.encrypt = false;

        esp_err_t err = esp_now_add_peer(&p);
        if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
            ESP_LOGE(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "Peer added: %s", CONFIG_J_ESPNOW_PEER_MAC);
    }

    ESP_LOGI(TAG, "ESPNOW started (node_id=%u)", (unsigned)CONFIG_J_NODE_ID);
    return ESP_OK;
}

static esp_err_t send_ctrl(uint8_t cmd, uint16_t value)
{
    if (!s_peer_ok) return ESP_ERR_INVALID_STATE;

    j_esn_ctrl_t m = {0};
    m.magic    = J_ESN_MAGIC;
    m.ver      = J_ESN_VER;
    m.type     = J_ESN_MSG_CTRL;
    m.src_node = (uint16_t)CONFIG_J_NODE_ID;
    m.dst_node = 0xFFFF; // пока broadcast; позже можно сделать конкретный node_id лампы
    m.seq      = s_seq++;
    m.cmd      = cmd;
    m.value_u16 = value;

    return esp_now_send(s_peer_mac, (const uint8_t*)&m, sizeof(m));
}

esp_err_t j_esn_send_power(bool on)               { return send_ctrl(J_ESN_CMD_POWER,      on ? 1 : 0); }
esp_err_t j_esn_send_pause(bool paused)          { return send_ctrl(J_ESN_CMD_SET_PAUSE,  paused ? 1 : 0); }
esp_err_t j_esn_send_brightness_u8(uint8_t b)    { return send_ctrl(J_ESN_CMD_SET_BRIGHT, (uint16_t)b); }
esp_err_t j_esn_send_speed_pct(uint16_t pct)     { return send_ctrl(J_ESN_CMD_SET_SPEED_PCT, pct); }
esp_err_t j_esn_send_anim_id(uint16_t effect_id) { return send_ctrl(J_ESN_CMD_SET_ANIM, effect_id); }
esp_err_t j_esn_send_ota_start(void)             { return send_ctrl(J_ESN_CMD_OTA_START, 0); }


bool j_esn_fx_cache_valid(void) { return peer0()->fx_valid; }


uint16_t j_esn_fx_cache_count(void)
{
    j_esn_peer_t *p = peer0();
    return p->fx_valid ? p->fx_count : 0;

}

uint16_t j_esn_fx_cache_id_by_index(uint16_t idx)
{
    j_esn_peer_t *p = peer0();
    if (!p->fx_valid) return 0;
    if (idx >= p->fx_count) return 0;
    return p->fx_entries[idx].id;
}


const char *j_esn_fx_cache_name_by_index(uint16_t idx)
{
    j_esn_peer_t *p = peer0();
    if (!p->fx_valid) return NULL;
    if (idx >= p->fx_count) return NULL;
    return p->fx_entries[idx].name;
}

uint16_t j_esn_fx_last_effect_id(void)
{
    return peer0()->last_effect_id;
}


esp_err_t j_esn_fx_sync_start(void)
{
    /* Safe even if no peer: just returns INVALID_STATE */
    return send_hello_meta_req();
}

void j_esn_fx_set_updated_cb(j_esn_fx_updated_cb_t cb, void *arg)
{
    s_fx_updated_cb = cb;
    s_fx_updated_arg = arg;
}

void j_esn_ota_set_updated_cb(j_esn_ota_updated_cb_t cb, void *arg)
{
    s_ota_updated_cb = cb;
    s_ota_updated_arg = arg;
}

bool j_esn_ota_info_valid(void)
{
    return (peer0()->ota_status == J_ESN_OTA_ST_READY) &&
           (peer0()->ota_ssid[0] != '\0') &&
           (peer0()->ota_pass[0] != '\0');
}

static void safe_copy_str(char *dst, size_t dst_sz, const char *src)
{
    if (!dst || dst_sz == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= dst_sz) n = dst_sz - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void j_esn_ota_get_info(char *ssid, size_t ssid_sz,
                        char *pass, size_t pass_sz,
                        uint8_t *status, uint16_t *ttl_s)
{
    j_esn_peer_t *p = peer0();
    if (status) *status = p->ota_status;
    if (ttl_s)  *ttl_s  = p->ota_ttl_s;

    safe_copy_str(ssid, ssid_sz, p->ota_ssid);
    safe_copy_str(pass, pass_sz, p->ota_pass);
}
