# Команды (сборка, прошивка, отладка)

## 1 Сборка (ESP-IDF)
Обычный цикл:

idf.py set-target esp32s3
idf.py build

Полная очистка:
idf.py fullclean
idf.py build

Прошивка:
idf.py -p COMx flash
idf.py -p COMx monitor

Меню конфигурации:
idf.py menuconfig

Монитор:
idf.py monitor

## 2 Git checkpoint (практика сессий)
Рекомендуемый порядок в конце сессии:
git status
git add -A
git commit -m "ui: <коротко что сделали>"
git push

Тег для бэкапа:
git tag -a <tag_name> -m "<tag message>"
git push origin <tag_name>

## 3 GitHub file size limit (remote_src.tgz)
`remote_src.tgz` не стоит коммитить:
- GitHub режет файлы > 100 MB
- артефакты/архивы лучше хранить отдельно или через LFS (если осознанно нужно)

Игнор (вариант точечно):
echo remote_src.tgz>> .gitignore

Если файл уже был добавлен в индекс:
git rm --cached remote_src.tgz

Потом:
git add .gitignore
git commit -m "git: ignore remote_src.tgz"
git push
