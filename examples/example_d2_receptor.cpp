// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// example_d2_receptor.cpp — Пример: метаботропный D2-подобный рецептор (тип 125, user).
// Активируется дофамином, снижает cAMP (Gi/o-эффект). Имеет пресинаптический
// ауторецепторный модулятор, снижающий выброс дофамина.
//
// Сборка (GCC):
//   g++ -std=c++17 -fPIC -shared -O2 -Wall
//       -I../../include -I../../BioNeuronCore/include
//       -o example_d2_receptor.so example_d2_receptor.cpp

#include "plugin-devkit.h"
#include <cmath>

// ==================== ХЕЛПЕРЫ ==============================================

// Gi/o-эффект: Gβγ активирует GIRK (K+ выходит), Gαi ингибирует аденилатциклазу.
static double gi_o_effect_user(Modifier* m, Segment* s, float dt, int nt_type) {
    (void)dt;

    float nt = s->neurotransmitter[nt_type];
    float EC50 = m->density * 100.0f;   // полуактивация
    if (EC50 < 0.01f) EC50 = 0.01f;

    float occupancy = nt / (nt + EC50);

    // GIRK-ток (K+ выходит → гиперполяризация)
    float E_K = biocore_nernst(s->intra_ions[ION_K], s->extra_ions[ION_K], 1.0f);
    float g_girk = 0.5f * occupancy * biocore_segment_area(s);
    float I_girk = g_girk * (s->V_m - E_K);

    return I_girk;   // положительный = выходящий K+ = гиперполяризация
}

// Модуляция выброса: снижение вероятности экзоцитоза
static float modulate_release_gi_user(Modifier* m, float* cleft_nt, float dt, int nt_type) {
    (void)dt;
    (void)nt_type;

    float base_reduction = 0.4f * m->density;
    if (base_reduction > 0.9f) base_reduction = 0.9f;

    *cleft_nt *= (1.0f - base_reduction);
    return 1.0f - base_reduction;
}

// ==================== ТОКОВАЯ ФУНКЦИЯ =====================================

static double d2_current(Modifier* m, Segment* s, float dt) {
    return gi_o_effect_user(m, s, dt, NT_DOPAMINE);
}

static float d2_modulate(Modifier* m, float* cleft_nt, float dt) {
    return modulate_release_gi_user(m, cleft_nt, dt, NT_DOPAMINE);
}

// ==================== ТОЧКИ ВХОДА ==========================================

BIOCORE_PLUGIN("Пример D2-рецептора", "1.0.0", "BioNeuronSDK") {
    BIOCORE_MOD_CURRENT(125, d2_current);
    BIOCORE_MOD_RELEASE(125, d2_modulate);
    return 2;
}

BIOCORE_DESC_FULL(
    "{"
    "\"name\":\"Пример D2-рецептора\","
    "\"version\":\"1.0.0\","
    "\"author\":\"BioNeuronSDK\","
    "\"modifiers\":["
        "{"
            "\"id\":125,"
            "\"name\":\"D2-подобный рецептор (пример)\","
            "\"category\":\"Метаботропные Gi/o\","
            "\"description\":\"Метаботропный дофаминовый D2-подобный рецептор с Gi/o-каскадом и пресинаптической модуляцией. Пример плагина BioNeuronSDK.\","
            "\"release\":true"
        "}"
    "]"
    "}"
)
