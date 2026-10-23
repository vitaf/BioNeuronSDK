// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// example_ca_channel.cpp — Пример: ионотропный Ca-канал (тип 124, user).
// Пропускает Ca2+ внутрь сегмента при деполяризации.
//
// Сборка (GCC):
//   g++ -std=c++17 -fPIC -shared -O2 -Wall
//       -I../../include -I../../BioNeuronCore/include
//       -o example_ca_channel.so example_ca_channel.cpp
//
// Тестирование:
//   Скопируйте example_ca_channel.so и .desc в plugins/ директорию BioNeuronCore.
//   Пересоберите dispatch_generated.cpp: biocore-gen-dispatch plugins/ -o dispatch_generated.cpp
//   Пересоберите Core и запустите симуляцию.

#include "plugin-devkit.h"
#include <cmath>

// ==================== ТОКОВАЯ ФУНКЦИЯ =====================================

static double ca_channel_current(Modifier* m, Segment* s, float dt) {
    (void)dt;

    float V = s->V_m;

    // Активация: сигмоида от напряжения
    float half_act = -20.0f;    // V полуактивации (мВ)
    float slope    = 5.0f;       // крутизна (мВ)
    float activation = 1.0f / (1.0f + expf(-(V - half_act) / slope));

    // Проводимость
    float area = biocore_segment_area(s);
    float g_max = m->density * area * activation;  // См

    // Равновесный потенциал Ca2+ (Нернст, valence=2)
    float E_Ca = biocore_nernst(s->intra_ions[ION_CA], s->extra_ions[ION_CA], 2.0f);

    float I_Ca = g_max * (V - E_Ca);

    // Перенос Ca2+ внутрь (FARADAY_CONST * объём)
    float volume = PI * s->radius * s->radius * s->length / 20.0f;
    float charge_per_mole = FARADAY_CONST * volume / 1000.0f;

    s->intra_ions[ION_CA] -= (I_Ca * dt) / charge_per_mole;
    s->extra_ions[ION_CA] += (I_Ca * dt) / (charge_per_mole * 10.0f);

    biocore_clamp_ions(s->intra_ions, ION_COUNT);
    biocore_clamp_ions(s->extra_ions, ION_COUNT);

    return -I_Ca;
}

// ==================== ТОЧКИ ВХОДА ==========================================

BIOCORE_PLUGIN("Пример Ca-канала", "1.0.0", "BioNeuronSDK") {
    BIOCORE_MOD_CURRENT(124, ca_channel_current);
    return 1;
}

BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"Пример Ca-канала\","
    "\"version\":\"1.0.0\","
    "\"author\":\"BioNeuronSDK\","
    "\"modifiers\":["
        "{"
            "\"id\":124,"
            "\"name\":\"Ca-канал (пример)\","
            "\"category\":\"Ионотропные\","
            "\"description\":\"Потенциал-зависимый кальциевый канал. Пример плагина BioNeuronSDK.\","
            "\"release\":false"
        "}"
    "]"
    "}"
)
