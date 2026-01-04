#pragma once
#include <stdint.h>

#define J_ESN_MAGIC   0x4A4E
#define J_ESN_VER     1

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
