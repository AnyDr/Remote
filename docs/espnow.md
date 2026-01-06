# ESP-NOW

Документ фиксирует **текущее фактическое состояние** ESP-NOW в проекте пульта.

## Goals
- Низкая задержка локального управления.
- Работает без AP/роутера (ESPNOW-only режим).
- Радио поднято как Wi-Fi STA, сверху работает ESP-NOW.

## Pairing / Identity (из конфигурации)
- `CONFIG_J_NODE_ID = 2` (пульт)
- `CONFIG_J_ESPNOW_PEER_MAC = "10:B4:1D:EA:DF:C8"` (peer, например лампа)
- `CONFIG_J_WIFI_FALLBACK_CH = 1` (фиксируем канал в ESPNOW-only режиме)

## Channel strategy
В `WIFI_Init()`:
1) стартуем Wi-Fi STA
2) **фиксируем канал**: `esp_wifi_set_channel(CONFIG_J_WIFI_FALLBACK_CH, ...)`
3) стартуем ESP-NOW link поверх STA

Скан Wi-Fi выключен (чтобы канал не “гулял”):
- `J_WIRELESS_SCAN_ENABLE = 0`

## Protocol: framing

### Константы
- `J_ESN_MAGIC = 0x4A4E`
- `J_ESN_VER   = 1`

### Типы сообщений (`type`)
- `CTRL  = 1`
- `ACK   = 2`
- `HELLO = 3` (зарезервировано/на будущее)

### Заголовок (часть CTRL/ACK)
Поля (в packed структурах):
- `magic` (u16)
- `ver` (u8)
- `type` (u8)
- `src_node` (u16)
- `dst_node` (u16) — сейчас в CTRL ставится `0xFFFF` (broadcast)
- `seq` / `ack_seq` (u32)

## CTRL message (`j_esn_ctrl_t`)
Поля команды:
- `cmd` (u8)
- `value_u16` (u16) — универсальное поле значения

Команды (`cmd`):
1) `J_ESN_CMD_POWER`
2) `J_ESN_CMD_SET_ANIM`
3) `J_ESN_CMD_SET_PAUSE`
4) `J_ESN_CMD_SET_BRIGHT`
5) `J_ESN_CMD_SET_SPEED_PCT`

Функции отправки на пульте:
- `j_esn_send_power(bool on)`
- `j_esn_send_pause(bool paused)`
- `j_esn_send_brightness_u8(uint8_t b)`
- `j_esn_send_speed_pct(uint16_t pct)`
- `j_esn_send_anim_id(uint16_t effect_id)`

## ACK message (`j_esn_ack_t`)
ACK содержит снапшот применённого состояния:
- `ack_seq` (u32)
- `effect_id` (u16)
- `brightness` (u8)
- `paused` (u8)
- `speed_pct` (u16)
- `state_seq` (u32)

### Текущее поведение в проекте
- Пульт **принимает ACK** и **логирует** его.
- ACK пока **не применяется** как механизм синхронизации UI state (только диагностика).

## FX Sync: синхронизация списка эффектов (кэш на пульте)
Цель: пульт должен знать корректные `effect_id` и имена эффектов так, как их знает лампа.

Фактическое поведение (по логам и текущей реализации):
- При наличии peer (лампы) пульт получает FX список чанками.
- В логе пульта видно прогресс вида:
  - `FX CHUNK: got 10 items, progress 40/75`
  - …
  - `FX cache saved: count=75 crc=0xXXXXXXXX`
  - `FX SYNC DONE: count=75 crc=0xXXXXXXXX`
- После успешного sync пульт использует кэш для:
  - отображения имён в selector
  - корректного отображения текущего эффекта на device screen

Примечание:
- Кэш хранится локально (persist), чтобы UI был осмысленным даже между перезагрузками.
- Источник истины всё равно лампа: при связи кэш должен обновляться.

## TODO (когда будем “делать по-взрослому”)
- Привязать ACK к UI state (single source of truth / state_seq).
- Добавить retry/timeout для CTRL (если ACK не пришёл).
- Разрулить `dst_node`: уйти от broadcast к конкретному node_id устройства.

## State
- Date: 2026-01-XX
- ESP-IDF: v5.5.1
- Remote MAC: 98:88:E0:03:D5:F4
- Peer (Lamp) MAC: 10:B4:1D:EA:DF:C8
- ESPNOW fallback channel: 1
- Node ID: 2
