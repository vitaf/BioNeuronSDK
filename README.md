# BioNeuronSDK — инструментарий разработчика плагинов

## Назначение

BioNeuronSDK — набор инструментов для создания пользовательских модификаторов (плагинов) для вычислительного движка **BioNeuronCore**. SDK позволяет разрабатывать ионные каналы, рецепторы нейромедиаторов, насосы и пресинаптические модуляторы без перекомпиляции ядра.

## Что даёт SDK

- **plugin-devkit.h** — единый заголовок со всеми типами, константами и хелперами. Подключил один файл — получил всё для разработки плагина.
- **biocore-gen-dispatch.py** — генератор GPU/SYCL dispatch-кода из `.desc` дескрипторов. Автоматизирует подключение плагина к GPU-пути.
- **CMake-модуль** — функции `biocore_add_plugin()` и `biocore_gen_dispatch()` для интеграции в сборочную систему.
- **Шаблоны и примеры** — готовая основа для создания плагина за 5 минут.

## Быстрый старт

### 1. Создайте плагин из шаблона

```bash
cp BioNeuronSDK/templates/plugin_template.cpp my_plugin.cpp
cp BioNeuronSDK/templates/plugin_template.desc my_plugin.desc
```

### 2. Отредактируйте шаблон

В `my_plugin.cpp`:
- Замените `M_PLUGIN_ID` на ID вашего типа (124–127)
- Реализуйте функцию тока `MyModCurrent(Modifier* m, Segment* s, float dt)`
- При необходимости добавьте пресинаптический модулятор `MyModRelease`
- Заполните метаданные в `BIOCORE_DESC_FULL`

В `my_plugin.desc`:
- Укажите `id`, `name`, `category`, `description`, `release`

### 3. Соберите .so

```bash
g++ -std=c++17 -fPIC -shared -O2 -Wall \
    -I<BioNeuronSDK>/include -I<BioNeuronCore>/include \
    -o my_plugin.so my_plugin.cpp
```

Важно: **только GCC**, не Intel dpcpp. Плагины загружаются через `dlopen` из Studio (GCC-сборка).

### 4. Установите плагин

Скопируйте `.so` и `.desc` в директорию плагинов BioNeuronCore:

```bash
cp my_plugin.so my_plugin.desc <BioNeuronCore>/plugins/
```

### 5. CPU-путь (без пересборки)

Плагин будет автоматически загружен ядром при старте симуляции. Функция тока вызывается через `modifier_current_table[]` — не требует пересборки Core.

### 6. GPU-путь (опционально)

Для GPU-ускорения пользовательского модификатора:

```bash
python3 BioNeuronSDK/src/biocore-gen-dispatch.py <plugins_dir>/ \
    --header -o dispatch_generated.h
```

Скопируйте `dispatch_generated.h` в `BioNeuronCore/src/`, предоставьте `static inline` версию функции в отдельном заголовке и пересоберите Core с dpcpp.

## Состав SDK

| Компонент | Файл | Назначение |
|-----------|------|-----------|
| DevKit | `include/plugin-devkit.h` | Типы (Modifier, Segment, Vec3), константы (ION_*, NT_*), макросы (BIOCORE_PLUGIN, BIOCORE_MOD_CURRENT, BIOCORE_MOD_RELEASE), хелперы (nernst, area, clamp) |
| Генератор | `src/biocore-gen-dispatch.py` | Сканирует `.desc`, генерирует `dispatch_generated.cpp` (CPU extern) или `dispatch_generated.h` (GPU inline) |
| CMake | `cmake/biocore-plugin.cmake` | `biocore_add_plugin()`, `biocore_gen_dispatch()`, `biocore_find_sdk()` |
| Шаблоны | `templates/` | `plugin_template.cpp` + `plugin_template.desc` — основа для нового плагина |
| Примеры | `examples/` | `example_ca_channel` (ионотропный) и `example_d2_receptor` (метаботропный + release) |

## API: ключевые макросы

```cpp
#include "plugin-devkit.h"

// Точка входа — регистрирует функции в Core
BIOCORE_PLUGIN("Имя", "1.0.0", "Автор") {
    BIOCORE_MOD_CURRENT(124, my_current_func);     // токовая функция
    BIOCORE_MOD_RELEASE(124, my_release_func);     // пресинаптический модулятор (опционально)
    return 2;                                     // количество зарегистрированных функций
}

// JSON-дескриптор плагина
BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"Мой Ca-канал\","
    "\"version\":\"1.0.0\","
    "\"author\":\"Автор\","
    "\"modifiers\":[{"
        "\"id\":124,"
        "\"name\":\"Ca-канал\","
        "\"category\":\"Ионотропные\","
        "\"description\":\"Потенциал-зависимый Ca2+ канал\","
        "\"release\":false"
    "}]"
    "}"
);
```

## Хелперы в plugin-devkit.h

| Функция | Назначение |
|---------|-----------|
| `biocore_nernst(intra, extra, valence)` | Равновесный потенциал по Нернсту (мВ) |
| `biocore_segment_area(seg)` | Площадь поверхности сегмента (μm²) |
| `biocore_clamp_ions(arr, count)` | Защита концентраций от ухода в ноль |

## Константы

- **Ионы:** `ION_NA` (0), `ION_K` (1), `ION_CA` (2), `ION_CL` (3), `ION_MG` (4)
- **Нейромедиаторы:** `NT_GLUTAMATE` (0) ... `NT_MELATONIN` (11), `NT_MAGNESIUM` (12), `NT_NICOTINE` (13), всего 14
- **Физика:** `FARADAY_CONST` (96485.0), `GAS_CONST` (8.314), `CELSIUS_37` (310.15K), `PI`

## Идентификаторы типов

| Диапазон | Назначение |
|----------|-----------|
| 0–123 | Встроенные типы (в `kernel_dispatch.h` Core) |
| **124–127** | **Пользовательские типы** (ваши плагины) |

## CPU vs GPU

| Путь | Механизм | Пересборка Core |
|------|----------|-----------------|
| **CPU** | `.so` → `dlopen` → `modifier_current_table[128]` | Нет |
| **GPU** | `dispatch_generated.h` с `static inline` → dpcpp | Да |

Пользовательские типы на GPU без сгенерированного dispatch возвращают 0.0f (нет тока). Для GPU-ускорения пользовательского плагина используйте `--header` режим генератора.

## Формат .desc

```json
{
  "name": "Имя плагина",
  "version": "1.0.0",
  "author": "Автор",
  "type_id": 124,
  "modifiers": [
    {
      "id": 93,
      "name": "Модификатор",
      "category": "Категория",
      "description": "Описание",
      "release": false
    }
  ]
}
```

Поля: `id` (124–127), `name`, `category`, `description`, `release` (true если есть пресинаптический модулятор).

## Тестирование

**Итого: 4 теста, 100% PASS.** Valgrind: утечек нет.

| Тест | Что проверяет | Проверок |
|------|--------------|----------|
| `test_plugin_devkit` | plugin-devkit.h: biocore_nernst, biocore_segment_area, biocore_clamp_ions, константы, размеры POD | 13 |
| `test_example_plugins` | dlopen/dlsym example плагинов, символы biocore_plugin_init/desc, .desc файлы | 4 |
| `test_cmake_module` | biocore-plugin.cmake: сборка .so, копирование .desc, gen_dispatch, find_sdk, без-DESC | 6 |
| `test_dispatch_gen` | Python-тесты biocore-gen-dispatch.py | генерация |

Запуск:
```bash
cd BioNeuronSDK/build
cmake .. -DBUILD_SDK_TESTS=ON && ctest --output-on-failure
```

---

## Права и интеллектуальная собственность

© 2024–2026 **Фешин Виталий Александрович** / Vitaly Alexandrovich Feshin. All rights reserved.
E-mail: feshin2004@mail.ru · vfeshin@mindcopy.ru · Тел.: +7(918)345-75-18. Краснодар, Россия / Krasnodar, Russia.

Свидетельство о государственной регистрации программы для ЭВМ (Роспатент) **№ 727894_683451**.
Коммерческое использование, распространение и сублицензирование без письменного разрешения автора запрещены.
Полный текст — см. `LICENSE` и `DISCLAIMER.md` в корне платформы.
