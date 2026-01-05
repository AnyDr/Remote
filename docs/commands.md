# Команды (сборка, прошивка, отладка)

## 1 Сборка (ESP-IDF)
Обычный цикл:

idf.py set-target esp32s3
idf.py build

Полная Очистка:
idf.py fullclean
idf.py build

Прошивка
idf.py -p COMx flash
idf.py -p COMx monitor

Меню конфигурации
idf.py menuconfig

Монитор:
idf.py monitor

## 2 Git checkpoint (ESP-IDF)
git status
git add docs
git commit -m "docs: update architecture/espnow/commands/handoff"
git push

Тег для бэкапа:
git tag -a docs_checkpoint -m "docs checkpoint"
git push origin docs_checkpoint