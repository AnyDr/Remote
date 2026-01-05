# ESP32 Remote (Touch LCD 1.46") for Jinny Lamp

Проект: пульт на ESP32-S3 с круглым тач-дисплеем 412x412 и UI на LVGL.
Основное назначение: управление устройствами (в т.ч. Jinny Lamp) через Wi-Fi (STA) + ESP-NOW.

Документация лежит в `docs/`:
- `architecture.md` – архитектура прошивки и UI-инварианты
- `espnow.md` – протокол/канал связи (ESP-NOW)
- `commands.md` – команды сборки/прошивки/отладки
- `Handoff.md` – “handoff”: текущее состояние, что работает, что осталось

## Железо (факты из кода)
Экран SPD2010, интерфейс QSPI (quad SPI):
- TE: GPIO18
- SCK: GPIO40
- D0..D3: GPIO46 / 45 / 42 / 41
- CS: GPIO21
- Backlight: GPIO5 (LEDC PWM)
Разрешение: 412x412, 16 bpp (RGB565). 

Touch контроллер SPD2010 по I2C:
- I2C addr: 0x53
- INT: GPIO4
- RST: -1 (reset/линия не на прямом GPIO; см. EXIO) 

I2C шина (общая для RTC/IMU/touch/EXIO):
- SDA: GPIO10
- SCL: GPIO11
- Частота: 400 kHz 

## Быстрый старт
Смотри `docs/commands.md`.

## Важно (инварианты управления)
- ESPNOW поверх поднятого Wi-Fi STA, канал фиксируется (в ESPNOW-only режиме). 
- Diag-экран должен оставаться последним в карусели устройств (правило insert-before-diag). 
- Оверлеи должны блокировать глобальный свайп карусели (см. `docs/architecture.md`).
