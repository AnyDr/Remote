#include "j_espnow_link.h"
#include "j_espnow_proto.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"

static const char *TAG = "J_ESN_R";

static uint32_t s_seq = 1;
static uint8_t  s_peer_mac[6] = {0};
static bool     s_peer_ok = false;

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

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!info || !data || len < 4) return;

    const uint16_t *pmagic = (const uint16_t*)data;
    if (*pmagic != J_ESN_MAGIC) return;

    if (len >= (int)sizeof(j_esn_ack_t)) {
        const j_esn_ack_t *a = (const j_esn_ack_t*)data;
        if (a->ver == J_ESN_VER && a->type == J_ESN_MSG_ACK) {
            ESP_LOGI(TAG, "ACK seq=%u effect=%u bright=%u paused=%u speed=%u",
                     (unsigned)a->ack_seq,
                     (unsigned)a->effect_id,
                     (unsigned)a->brightness,
                     (unsigned)a->paused,
                     (unsigned)a->speed_pct);
        }
    }
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
