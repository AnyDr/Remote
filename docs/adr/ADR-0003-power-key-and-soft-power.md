# ADR-0003: Power key — soft power via GPIO6/7, long-press thresholds

## Context
Плата имеет:
- вход кнопки питания,
- управляющий пин “power control” (soft power),
и нужно безопасно/предсказуемо:
- включать устройство,
- обрабатывать long-press (sleep/restart/shutdown).

## Decision
- Используем:
  - `PWR_KEY_Input_PIN = GPIO6`
  - `PWR_Control_PIN   = GPIO7`
- Логика:
  - после init, при нажатой кнопке включаем `PWR_Control_PIN`
  - long-press режимы задаются порогами:
    - `Device_Sleep_Time`
    - `Device_Restart_Time`
    - `Device_Shutdown_Time`
- Пороговые значения — **счётчик итераций `PWR_Loop()`**, т.е. зависят от частоты вызова.

## Consequences
Плюсы:
- Простая и надёжная схема без сложных таймеров.
- Поведение легко диагностировать логами/отладкой.

Минусы/долги:
- Нужно явно зафиксировать частоту вызова `PWR_Loop()` (иначе “10” может быть и 100 мс, и 2 секунды).
- Для стабильных таймингов лучше перейти на `pdMS_TO_TICKS()` или esp_timer, когда будем “цементировать” продуктовую логику питания.
