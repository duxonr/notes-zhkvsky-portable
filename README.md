# Notes ZHKVSKY Portable

Windows x64 Portable-версия на базе [официального Notes](https://github.com/nuttyartist/notes).
Текущая зафиксированная версия форка: **v0.2.0**. Версия исходного приложения: 2.3.1.

## Установка через Codex на другом ПК

[Готовое задание для Codex](DEPLOY_WITH_CODEX.md) — скопируйте текст инструкции в чат на целевом Windows-ПК.

## Запуск

1. Скачайте `Notes_ZHKVSKY_Portable_v0.2.0_Windows_x64.zip` из Releases.
2. Распакуйте папку целиком в доступное для записи место.
3. Запустите `Notes.exe`. Установка программы и Qt не нужна.

Рядом с программой создаются `Data/notes.db`, `Data/Attachments`, `Data/Thumbnails` и `Settings/Settings.ini`.
В опубликованном архиве нет пользовательских заметок и настроек.
Для переноса на другой ПК копируйте всю папку. При обновлении сохраняйте свои `Data` и `Settings`.
Перед обновлением полностью закройте Notes, включая значок в трее, и сделайте резервную копию.

## Возможности сборки

- Portable-хранилище и независимый экземпляр для каждой папки программы.
- Pro/Kanban включены официальной опцией исходной сборки; проверка обновлений отключена.
- Форматирование, теги, фотокарточки с оригиналами, копированием и перемещением.
- В callout: Ctrl+Enter продолжает блок, Enter выходит с отступом.
- Рамка callout намеренно расположена левее общей границы текста: фотокарточка внутри не смещается.

Подробности и ограничения: [состояние v0.2.0](docs/releases/v0.2.0.md).

## Сборка

Нужны MSVC 2022 x64, Windows SDK, Qt 6.8.3 MSVC 2022 64-bit, CMake и Ninja.
Команды выполняются в x64 Native Tools Command Prompt:

```bat
git clone --recurse-submodules https://github.com/duxonr/notes-zhkvsky-portable.git
cd notes-zhkvsky-portable
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 -DNOTES_PORTABLE=ON -DPRO_VERSION=ON -DUPDATE_CHECKER=OFF -DGIT_REVISION=OFF
cmake --build build --parallel 4
```

После сборки скопируйте Notes.exe в отдельную папку и выполните `windeployqt --release --qmldir src <путь-к-Notes.exe>`.
Используйте windeployqt из той же версии Qt. Из полного source ZIP в Releases submodules уже доступны без отдельной загрузки.
См. также [оригинальные инструкции Windows](docs/build_on_windows.md).

## Происхождение и лицензии

Сохранены история и авторство upstream Notes, файл [LICENSE](LICENSE) (MPL-2.0), лицензии сторонних компонентов и pinned submodules.
Оригинальное описание: [Upstream README](docs/UPSTREAM_README.md).
