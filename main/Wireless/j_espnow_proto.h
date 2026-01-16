#pragma once
#include <stdint.h>

#define J_ESN_MAGIC   0x4A4E
#define J_ESN_VER     1

/* =========================
 *  HELLO: OTA INFO (SoftAP)
 * ========================= */

#define J_ESN_OTA_SSID_MAX   32   /* SSID max per Wi-Fi spec */
#define J_ESN_OTA_PASS_MAX   63   /* WPA2 passphrase max */

/* =========================
 *  HELLO: FX LIST SYNC
 * ========================= */

#define J_ESN_FX_NAME_MAX      16   // фиксированная длина имени (0-terminated, padded)
#define J_ESN_FX_CHUNK_MAX     10   // 10 * (2+16)=180 bytes + header ~= 200 bytes (OK for ESPNOW)

/* Общий заголовок всех сообщений (первые поля совпадают у CTRL/ACK/HELLO) */
typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  ver;
    uint8_t  type;     // j_esn_msg_type_t
    uint16_t src_node;
    uint16_t dst_node; // 0xFFFF = broadcast
    uint32_t seq;
} j_esn_hdr_t;

typedef enum : uint8_t {
    J_ESN_HELLO_FX_META_REQ      = 1,
    J_ESN_HELLO_FX_META_RSP      = 2,
    J_ESN_HELLO_FX_CHUNK_REQ     = 3,
    J_ESN_HELLO_FX_CHUNK_RSP     = 4,
    /* OTA: lamp -> remote (SoftAP SSID + PASS, read-only on remote) */
    J_ESN_HELLO_OTA_INFO_RSP     = 5,
} j_esn_hello_cmd_t;


/* META request: просто спросить count/crc32 */
typedef struct __attribute__((packed)) {
    j_esn_hdr_t h;         // type = J_ESN_MSG_HELLO
    uint8_t  hello_cmd;    // J_ESN_HELLO_FX_META_REQ
    uint8_t  rsv0;
    uint16_t rsv1;
    uint32_t rsv2;
} j_esn_fx_meta_req_t;

/* META response: count + crc32 */
typedef struct __attribute__((packed)) {
    j_esn_hdr_t h;         // type = J_ESN_MSG_HELLO
    uint8_t  hello_cmd;    // J_ESN_HELLO_FX_META_RSP
    uint8_t  rsv0;
    uint16_t fx_count;
    uint32_t fx_crc32;
} j_esn_fx_meta_rsp_t;

/* CHUNK request: стартовый индекс */
typedef struct __attribute__((packed)) {
    j_esn_hdr_t h;         // type = J_ESN_MSG_HELLO
    uint8_t  hello_cmd;    // J_ESN_HELLO_FX_CHUNK_REQ
    uint8_t  rsv0;
    uint16_t start_index;
    uint32_t rsv1;
} j_esn_fx_chunk_req_t;

/* Одна запись эффекта */
typedef struct __attribute__((packed)) {
    uint16_t id;
    char     name[J_ESN_FX_NAME_MAX];
} j_esn_fx_entry_t;

/* CHUNK response: start_index + count + entries[] */
typedef struct __attribute__((packed)) {
    j_esn_hdr_t h;         // type = J_ESN_MSG_HELLO
    uint8_t  hello_cmd;    // J_ESN_HELLO_FX_CHUNK_RSP
    uint8_t  count;        // 1..J_ESN_FX_CHUNK_MAX
    uint16_t start_index;
    uint32_t fx_crc32;     // чтобы remote мог отбрасывать “не тот список”
    j_esn_fx_entry_t entries[J_ESN_FX_CHUNK_MAX];
} j_esn_fx_chunk_rsp_t;


typedef enum : uint8_t {
    J_ESN_OTA_ST_NONE  = 0,
    J_ESN_OTA_ST_READY = 1,  /* SSID/PASS valid */
    J_ESN_OTA_ST_BUSY  = 2,
    J_ESN_OTA_ST_ERROR = 3,
} j_esn_ota_status_t;

typedef struct __attribute__((packed)) {
    j_esn_hdr_t h;          /* type = J_ESN_MSG_HELLO */
    uint8_t     hello_cmd;  /* J_ESN_HELLO_OTA_INFO_RSP */
    uint8_t     ota_status; /* j_esn_ota_status_t */
    uint16_t    ttl_s;      /* optional, 0 = unknown/unused */
    char        ssid[J_ESN_OTA_SSID_MAX + 1];
    char        pass[J_ESN_OTA_PASS_MAX + 1];
} j_esn_ota_info_rsp_t;


typedef enum : uint8_t {
    J_ESN_MSG_CTRL  = 1,
    J_ESN_MSG_ACK   = 2,
    J_ESN_MSG_HELLO = 3,
} j_esn_msg_type_t;

typedef enum : uint8_t {
    J_ESN_CMD_POWER = 1,
    J_ESN_CMD_SET_ANIM,
    J_ESN_CMD_SET_PAUSE,
    J_ESN_CMD_SET_BRIGHT,
    J_ESN_CMD_SET_SPEED_PCT,

    /* OTA session start (lamp will switch to SoftAP and later send HELLO_OTA_INFO_RSP) */
    J_ESN_CMD_OTA_START,
} j_esn_cmd_t;


typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  ver;
    uint8_t  type;
    uint16_t src_node;
    uint16_t dst_node;     // 0xFFFF = broadcast
    uint32_t seq;

    uint8_t  cmd;
    uint8_t  rsv0;
    uint16_t value_u16;

    uint32_t rsv1;
} j_esn_ctrl_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  ver;
    uint8_t  type;
    uint16_t src_node;
    uint16_t dst_node;
    uint32_t ack_seq;

    uint16_t effect_id;
    uint8_t  brightness;
    uint8_t  paused;
    uint16_t speed_pct;
    uint32_t state_seq;
} j_esn_ack_t;
