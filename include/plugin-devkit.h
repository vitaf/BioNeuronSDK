// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// plugin-devkit.h — BioNeuronSDK: инструментарий разработчика плагинов.
// Включается в каждый плагин. Предоставляет типы, хелперы и макросы
// для регистрации модификаторов в BioNeuronCore.
//
// Использование:
//   #include "plugin-devkit.h"
//   BIOCORE_PLUGIN("MyPlugin", "1.0.0", "Author") {
//       BIOCORE_MOD_CURRENT(64, my_current_func);
//       BIOCORE_MOD_RELEASE(64, my_release_func);
//       return 2;
//   }
//   BIOCORE_DESC_FULL("{...}");
//
// Сборка плагина:
//   g++ -std=c++17 -fPIC -shared -I<bio-core>/include -I<bio-sdk>/include -o my.so my_plugin.cpp

#pragma once
#include <cstdint>
#include <cmath>

// ==================== ТИПЫ (совместимы с BioNeuronCore/include/types.h) ================

#define ION_NA      0
#define ION_K       1
#define ION_CA      2
#define ION_CL      3
#define ION_MG      4    // Mg²⁺ — для NMDA Mg²⁺-блока
#define ION_A       5    // A⁻ — неподвижный внутриклеточный макроанион, z=−1, D=0
#define ION_H       6    // H⁺ — протон (pH-зависимые каналы: OTOP1), z=+1
#define ION_COUNT   7    // активное число ионов
#define ION_MAX     8    // физический максимум слотов (запас на будущее)

#define NT_GLUTAMATE       0
#define NT_GABA            1
#define NT_DOPAMINE        2
#define NT_SEROTONIN       3
#define NT_ACETYLCHOLINE   4
#define NT_ADENOSINE       5
#define NT_NORADRENALINE   6
#define NT_GLYCINE         7
#define NT_HISTAMINE       8
#define NT_ENDOCANNABINOID 9
#define NT_ATP             10
#define NT_MELATONIN       11
#define NT_MAGNESIUM       12   // Mg²⁺ в синаптической щели
#define NT_NICOTINE        13   // никотин для β2-nAChR
#define NT_CAPSAICIN       14   // капсаицин — экзогенный агонист TRPV1
#define NT_COUNT           15

#define MAX_STATE_VARS                    8
#define MAX_MODIFIERS_PER_SEGMENT         32
#define MAX_INTRACONNECTIONS_PER_SEGMENT  16

#define FARADAY_CONST   96485.0f
#define GAS_CONST       8.314f
#define CELSIUS_37      310.15f
#define PI              3.14159265f

#pragma pack(push, 1)

typedef struct { float x, y, z; } Vec3;

typedef struct {
    uint8_t  type;
    uint8_t  _pad[7];         // выравнивание density (double) до offset 8
    double   density;
    double   state[MAX_STATE_VARS];
    uint8_t  active_vars;
    uint8_t  flags;
    uint8_t  _reserved[6];    // хвост до 88 (кратность 8)
} Modifier;

typedef struct {
    uint32_t id;
    uint32_t neuron_id;
    Vec3     position;
    Vec3     direction;
    float    length;
    float    radius;
    double   V_m;
    double   intra_ions[ION_MAX];
    double   extra_ions[ION_MAX];
    float    intra_nt[NT_COUNT];
    float    neurotransmitter[NT_COUNT];
    Modifier modifiers[MAX_MODIFIERS_PER_SEGMENT];
    uint8_t  modifier_count;
    uint8_t  segment_type;
    uint8_t  reserved1[6];  // v14: +4 байта — выравнивание I_buffer (double) на 8
    float    ATP_level;
    float    temperature;
    double   I_buffer;
    float    transcription_signal;  // v5: ядерный CREB/транскрипционный сигнал (0..1)
    uint32_t intra_connection_ids[MAX_INTRACONNECTIONS_PER_SEGMENT];
    uint8_t  intra_connection_count;
    uint8_t  reserved2[7];  // v14: +4 байта — sizeof кратен 8
    float    ER_Ca2;                // v10: [Ca²⁺] в эндоплазматическом ретикулуме (mM)
} Segment;

#pragma pack(pop)

static_assert(sizeof(Modifier) == 88,  "Modifier must be 88 bytes (match Core types.h v13)");
static_assert(sizeof(Segment)  == 3216, "Segment must be 3216 bytes (v14: I_buffer 8-aligned)");

// ==================== СИГНАТУРЫ ФУНКЦИЙ =============================================

// Функция тока модификатора: возвращает ток (положительный = деполяризация)
typedef double (*ModifierFunc)(Modifier* mod, Segment* seg, float dt);

// Функция пресинаптического модулятора: модифицирует cleft_neurotransmitter
typedef float (*ReleaseModulatorFunc)(Modifier* mod, float* cleft_nt, float dt);

// ==================== API РЕГИСТРАЦИИ (вызывается из biocore_plugin_init) ==============

// Эти функции предоставляются BioNeuronCore. Плагин вызывает их
// в biocore_plugin_init() для регистрации своих обработчиков.
// extern "C" — обязательно для dlopen (плагин ищет символы в хосте).
#ifdef __cplusplus
extern "C" {
#endif

void register_modifier_current(uint8_t type, ModifierFunc func);
void register_modulator_release(uint8_t type, ReleaseModulatorFunc func);

#ifdef __cplusplus
}
#endif

// ==================== МАКРОСЫ ДЛЯ УПРОЩЕНИЯ РАЗРАБОТКИ ===============================

// BIOCORE_PLUGIN — объявляет точку входа biocore_plugin_init().
// Использование: BIOCORE_PLUGIN("Имя", "1.0.0", "Автор") { ... }
// Дескриптор задаётся отдельно через BIOCORE_DESC_FULL().
#define BIOCORE_PLUGIN(name, version, author) \
    extern "C" int biocore_plugin_init()

// BIOCORE_MOD_CURRENT — регистрирует токовую функцию модификатора
#define BIOCORE_MOD_CURRENT(type_id, func) \
    register_modifier_current((type_id), (func))

// BIOCORE_MOD_RELEASE — регистрирует пресинаптический модулятор
#define BIOCORE_MOD_RELEASE(type_id, func) \
    register_modulator_release((type_id), (func))

// BIOCORE_DESC — возвращает JSON-дескриптор. Требует ручного определения _biocore_plugin_desc_json.
// Для автоматического определения используйте BIOCORE_DESC_FULL().
#define BIOCORE_DESC() \
    extern "C" const char* biocore_plugin_desc() { return _biocore_plugin_desc_json; }

// BIOCORE_DESC_FULL — полный дескриптор с информацией о модификаторах (без макросов)
// Используйте эту форму для плагинов с детальным описанием модификаторов.
// Пример:
//   BIOCORE_DESC_FULL(
//     "{"
//     "\"name\":\"My Ca Channel\","
//     "\"version\":\"1.0.0\","
//     "\"author\":\"Developer\","
//     "\"modifiers\":["
//       "{\"id\":121,\"name\":\"Ca-канал\",\"category\":\"Ионотропные\",\"description\":\"...\",\"release\":false}"
//     "]"
//     "}"
//   )
#define BIOCORE_DESC_FULL(json_string) \
    static const char _biocore_plugin_desc_json[] = json_string; \
    BIOCORE_DESC()

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =========================================

// Вычисление равновесного потенциала по Нернсту (мВ)
static inline float biocore_nernst(float intra, float extra, float valence) {
    if (intra < 1e-6f) intra = 1e-6f;
    if (extra < 1e-6f) extra = 1e-6f;
    return (GAS_CONST * CELSIUS_37) / (valence * FARADAY_CONST) * logf(extra / intra) * 1000.0f;
}

// Площадь поверхности сегмента (radius и length в μm, площадь в μm²)
static inline float biocore_segment_area(const Segment* s) {
    return 2.0f * PI * s->radius * s->length;
}

// Защита концентраций от ухода в ноль
static inline void biocore_clamp_ions(float* arr, int count) {
    for (int i = 0; i < count; i++) {
        if (arr[i] < 1e-6f) arr[i] = 1e-6f;
    }
}
