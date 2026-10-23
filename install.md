# Установка и настройка BioNeuronSDK

## Системные требования

- Linux x86_64
- Python 3.6+
- GCC 9+ (для сборки плагинов)
- BioNeuronCore (установлен или собран из исходников)
- CMake 3.20+ (опционально, для cmake-модуля)

## Установка из .deb пакета

```bash
sudo dpkg -i mindcopy-sdk_0.6.0_amd64.deb
```

Файлы устанавливаются в `/opt/mindcopy/`:
- `include/plugin-devkit.h` — заголовок для разработки плагинов
- `bin/biocore-gen-dispatch.py` — генератор dispatch-кода
- `share/sdk/cmake/` — CMake-модуль `biocore-plugin.cmake`
- `share/sdk/templates/` — шаблоны плагинов
- `share/sdk/examples/` — примеры плагинов

## Загрузка плагинов (локально / через демон)

Плагины компилируются как `.so` файлы и загружаются через `dlopen`:

- **Локально (STATIC-режим):** Studio линкует `libbiocore_gcc.a` и загружает плагины в своём процессе
- **Удалённо (IPC-режим):** демон `biocore-daemon` загружает плагины в своём процессе. Studio подключается к демону через Unix-сокет или TCP (порт 6789 по умолчанию)

API плагинов (`extern "C" register_modifier_current / register_modulator_release`) одинаков для обоих режимов — плагинам не нужно знать, в каком процессе они загружены.

## Переменные окружения

- `BIOCORE_ROOT` — путь к BioNeuronCore (опционально)
- `BIOCORE_SDK_DIR` — путь к SDK (опционально)

## GPU-совместимость плагинов

Плагины компилируются GCC и загружаются через dlopen — GPU-бекенд не влияет на плагины.
Выбор GPU-бекенда для Core — см. [Core install.md](../BioNeuronCore/install.md#выбор-gpu-бекенда).

## Верификация

```bash
python3 /opt/mindcopy/bin/biocore-gen-dispatch.py --stub
```


## Права и интеллектуальная собственность

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Тел.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Свидетельство о государственной регистрации программы для ЭВМ (Роспатент) **№ 727894_683451**.
Коммерческое использование, распространение и сублицензирование без письменного разрешения автора запрещены.
Полный текст — см. `LICENSE` и `DISCLAIMER.md` в корне платформы.
