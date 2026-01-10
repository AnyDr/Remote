# ADR-0004: UI — Center gestures: double-tap selector, hold=pause/power (no single-tap action)

## Context
На device screen была проблема UX:
- “pause по любому короткому тапу” вызывал случайные паузы и раздражал.
Также нужно было:
- оставить double tap для входа/выхода в selector
- сделать предсказуемое выключение (power) без случайных триггеров

## Decision
- В центре device screen фиксируем жесты:
  - single короткий тап: ничего
  - double tap по центру: вход/выход в animation selector
  - hold >= `J_PAUSE_HOLD_MS`: toggle pause
  - hold >= `J_POWER_HOLD_MS`: toggle power
- Реализация через `LV_EVENT_PRESSED/RELEASED/PRESS_LOST` и измерение `lv_tick_elaps()`,
  а не через `LV_EVENT_LONG_PRESSED`, чтобы пороги были в ms и позволяли 5 секунд.
- Центр ограничен радиусом (“около 1 см”), чтобы жесты не срабатывали от касаний вне центральной зоны.

## Consequences
Плюсы:
- Убрали случайные паузы от коротких тапов.
- Power стал “долго и осознанно”, без сюрпризов.
- Double tap остаётся быстрым входом/выходом в selector.

Минусы/долги:
- Порог `J_PAUSE_HOLD_MS` требует тюнинга под реальную эргономику.
- Нужно следить, чтобы overlay/selector “съедали” события и не пробрасывали их подложке.
