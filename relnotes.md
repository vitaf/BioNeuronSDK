# Release Notes — BioNeuronSDK

## v0.6.2 (2026-07-01) — Agent Skills + dispatch fix
- **Agent Skills**: методология DDD + debugging triage (SDK-специфика: dispatch, dlopen, GCC-only) + planning workflow в CLAUDE.md
- **biocore-gen-dispatch fix**: генерирует weak symbols вместо extern declarations — предотвращает конфликты при линковке
- **GPU compatibility**: документирована совместимость GPU backends в install.md
- **Совместимость с Core v0.19.5** (CUDA two-pass diffusion fix)

## v0.6.1 (2026-06-25) — Совместимость с Core v0.19.4
- Совместимость с единым лог-форматом unixtime [LEVEL] msg
- Явное указание GCC-сборки плагинов (без Intel-зависимостей)

## v0.6.0 — BION v6 + итоговый сдвиг пользовательских ID

**Дата:** 16 июня 2026

- **MOD_TYPE_COUNT=121** (Core v0.16.7). Core добавил сенсорные модификаторы (115-120).
- **Диапазон пользовательских ID: 121–127** (был 93–127).
- **BION v6 полная совместимость**: `Nucleus` 332 байта, `NetworkHeader` 84 байта, `Segment` 988 байт.
- Все struct static_assert совпадают с Core v0.16.7.
- Примеры и шаблоны обновлены: ID 121 (Ca-канал), ID 122 (D2-рецептор).
- **Тесты (18.06.2026):** +2 теста — `test_sdk_security` (валидация .desc JSON, 8 проверок), `test_sdk_integration` (полный цикл плагина, 3 проверки). SDK: 6/6 тестов.

## v0.5.0-v0.4.0 — Тесты CMake-модуля + сдвиг на MOD_TYPE_COUNT=107

**Дата:** 10 июня 2026

**v0.5.0:**
- test_cmake_module.cpp: 6 тестов biocore-plugin.cmake (сборка .so, dispatch, find_sdk)
- test_example_plugins.cpp: 4 теста dlopen/dlsym плагинов
- Итого: 4/4 тестов, 100% PASS
- Valgrind: утечек нет

**v0.4.0:**
- **MOD_TYPE_COUNT=107** (было 98). Core добавил 9 новых типов (98-106).
- **Диапазон пользовательских ID: 107–127** (был 98–127). Сдвиг из-за новых встроенных типов гомеостаза и каналов.
- Новые встроенные типы (98-106): PMCA, NCX, KCC2, Ca_Leak, Ca_Buffer, SK, BK, NaP, Kir
- Совместимость с Core v0.13.0 и Studio v0.20.0

**Дата:** 10 июня 2026

- test_cmake_module.cpp: 6 тестов biocore-plugin.cmake (сборка .so, dispatch, find_sdk)
- test_example_plugins.cpp: 4 теста dlopen/dlsym плагинов
- Итого: 4/4 тестов, 100% PASS
- Valgrind: утечек нет

## v0.4.0 — Совместимость с Core v0.13.0

**Дата:** июнь 2026

- **MOD_TYPE_COUNT=107** (было 98). Core добавил 9 новых типов (98-106).
- **Диапазон пользовательских ID: 107–127** (был 98–127). Сдвиг из-за новых встроенных типов гомеостаза и каналов.
- Новые встроенные типы (98-106): PMCA, NCX, KCC2, Ca_Leak, Ca_Buffer, SK, BK, NaP, Kir
- Все новые типы: GHK + прямое движение ионов (без I=g*(V-E_rev))
- Совместимость с Core v0.13.0 и Studio v0.20.0

## v0.3.0 — Совместимость с Core v0.11.0

**Дата:** июнь 2026

- Диапазон пользовательских ID: 93–127 (Core MOD_TYPE_COUNT=98)
- **Segment fix**: добавлен `intra_nt[NT_COUNT]` (56 байт) — бинарная совместимость 988 байт с Core
- `MAX_INTRACONNECTIONS_PER_SEGMENT` (16) вместо магического числа
- `static_assert(sizeof(Segment) == 988)` и `sizeof(Modifier) == 44)` для защиты от рассинхронизации
- Примеры: ID 93 (Ca-канал), ID 94 (D2-рецептор)

## v0.2.0 — Совместимость с Core v0.10.0

**Дата:** июнь 2026

- Диапазон пользовательских ID: 93–127 (был 64–127)
- Встроенные типы расширены до 0–92 (добавлены модификаторы консолидации 87-92)
- Совместимость с BION v5 и структурами Nucleus

## v0.1.0 — Первый релиз SDK

**Дата:** май 2026

### Общее
- `VERSION` — файл с версией SDK (0.1.0), читается Studio для отображения

### plugin-devkit.h
- Единый заголовочный файл для разработки плагинов
- Все POD-типы: `Modifier`, `Segment`, `Vec3` с `#pragma pack(1)`
- Константы: 5 ионов (`ION_*`), 14 нейромедиаторов (`NT_*`), физические (`FARADAY_CONST`, `GAS_CONST`, `CELSIUS_37`, `PI`)
- `extern "C"` API регистрации: `register_modifier_current()`, `register_modulator_release()`
- Макросы: `BIOCORE_PLUGIN`, `BIOCORE_MOD_CURRENT`, `BIOCORE_MOD_RELEASE`, `BIOCORE_DESC_FULL`
- Хелперы: `biocore_nernst()`, `biocore_segment_area()`, `biocore_clamp_ions()`

### biocore-gen-dispatch.py
- Генератор `dispatch_generated.cpp` из `.desc` файлов
- Два режима: `.cpp` (CPU, extern) и `.h` (GPU, static inline)
- Режим `--stub` для генерации заглушки без плагинов
- Поддержка директории плагинов через `--plugins-dir`
- Фильтрация пользовательских типов (64–127), встроенные (0–62) пропускаются
- Автоопределение output через `--core-path`

### biocore-plugin.cmake
- `biocore_add_plugin()` — сборка .so плагина через GCC (не Intel dpcpp)
- `biocore_gen_dispatch()` — запуск генератора dispatch
- `biocore_find_sdk()` — автоопределение путей SDK
- Поддержка `DESC` и `OUTPUT_DIR` параметров
- Агрегатная цель `user_plugins`

### Шаблоны
- `plugin_template.cpp` — основа плагина с макросами SDK
- `plugin_template.desc` — JSON-дескриптор

### Примеры
- `example_ca_channel` — ионотропный Ca²⁺ канал (тип 64, без release)
- `example_d2_receptor` — метаботропный D2-подобный рецептор (тип 65, с release-модулятором)
- CMakeLists.txt для сборки примеров

### Документация
- `read_me.md` — описание SDK и quickstart
- `install.md` — инструкция по установке и интеграции
- `CLAUDE.md` — техническая документация для AI-ассистента

### Интеграция
- Совместимость с BioNeuronCore v0.6.0
- Поддержка в BioNeuronStudio (настройка путей, кнопка пересборки dispatch, отображение плагинов)
- Диапазон ID: 64–127 для пользовательских типов
- CPU-путь через .so плагины — без пересборки Core
- GPU-путь через dispatch_generated.h — запланирован для будущих версий


## Права и интеллектуальная собственность

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Тел.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Свидетельство о государственной регистрации программы для ЭВМ (Роспатент) **№ 727894_683451**.
Коммерческое использование, распространение и сублицензирование без письменного разрешения автора запрещены.
Полный текст — см. `LICENSE` и `DISCLAIMER.md` в корне платформы.
