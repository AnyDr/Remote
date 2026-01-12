# Команды (сборка, прошивка, отладка)

## 1 Сборка (ESP-IDF)
Обычный цикл:

cd "D:\esp\ESP32-S3-Touch-LCD-Remote"
idf.py build

Полная очистка:
idf.py fullclean
idf.py build

Прошивка:
idf.py -p COM11 flash
idf.py -p COM11 monitor

Меню конфигурации:
idf.py -p COM11 menuconfig

## 2 Git checkpoint (практика сессий)
Рекомендуемый порядок в конце сессии:

cd D:\esp\ESP32-S3-Touch-LCD-Remote
git status
git add -A
git commit -m "вписать инфомационный тег/якорь на агл"

git tag -a (backup/<on\off> -m "<msg>") - ??    
git push origin (backup/<name>)  - ??


git push origin stable/work     переключение на ветку стейбл
git branch --show-current       покажет ветку, статус файлов и т.д.
git status
git log --oneline -n 1


git switch stable/work - переключение между ветками

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
 трыньк