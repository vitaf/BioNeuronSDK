// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// Шаблон плагина BioNeuronCore. Используйте этот файл как основу.
//
// Что заменить:
//   - M_PLUGIN_ID      — ID типа (124-127 для пользовательских)
//   - MyModCurrent      — имя вашей функции тока
//   - MyModRelease      — имя вашей функции модуляции выброса
//   - "ИмяПлагина"      — название в дескрипторе
//
// Сборка:
//   g++ -std=c++17 -fPIC -shared -O2 -Wall
//       -I<BioNeuronSDK>/include -I<BioNeuronCore>/include
//       -o my_plugin.so my_plugin.cpp
//
// Или через CMake:
//   biocore_add_plugin(my_plugin my_plugin.cpp DESC my_plugin.desc)

#include "plugin-devkit.h"
#include <cmath>

// ==================== ТОКОВАЯ ФУНКЦИЯ МОДИФИКАТОРА ====================

static float MyModCurrent(Modifier* m, Segment* s, float dt) {
    // Пример: пассивная утечка с заданной проводимостью
    float area = biocore_segment_area(s);
    float g = m->density * area;          // проводимость (См)
    float E_leak = -65.0f;                // потенциал покоя (мВ)
    float V = s->V_m;

    float I = g * (V - E_leak);
    return -I;                            // отрицательный = гиперполяризация
}

// ==================== ПРЕСИНАПТИЧЕСКИЙ МОДУЛЯТОР (опционально) ==========

static float MyModRelease(Modifier* m, float* cleft_nt, float dt) {
    // Пример: снижение выброса нейромедиатора на 20%
    float mod = 1.0f - 0.2f * m->density;
    if (mod < 0.1f) mod = 0.1f;
    return mod;
}

// ==================== ТОЧКИ ВХОДА =======================================

BIOCORE_PLUGIN("ИмяПлагина", "1.0.0", "Автор") {
    BIOCORE_MOD_CURRENT(124, MyModCurrent);
    BIOCORE_MOD_RELEASE(124, MyModRelease);   // удалите если не нужен
    return 2;                                // количество зарегистрированных функций
}

BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"ИмяПлагина\","
    "\"version\":\"1.0.0\","
    "\"author\":\"Автор\","
    "\"modifiers\":["
        "{"
            "\\"id\\":124,"
            "\"name\":\"Мой модификатор\","
            "\"category\":\"Пользовательские\","
            "\"description\":\"Описание моего модификатора\","
            "\"release\":true"
        "}"
    "]"
    "}"
)
