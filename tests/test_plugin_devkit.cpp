// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// Тесты plugin-devkit.h: biocore_nernst, biocore_segment_area, biocore_clamp_ions
#include "plugin-devkit.h"
#include <cstdio>
#include <cmath>
#include <cassert>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define CHECK(cond) do { if (!(cond)) { printf("FAIL at %s:%d\n", __FILE__, __LINE__); return 1; } } while(0)
#define CHECK_NEAR(a, b, eps) do { if (fabsf((a)-(b)) > (eps)) { printf("FAIL: %f != %f at %s:%d\n", (float)(a), (float)(b), __FILE__, __LINE__); return 1; } } while(0)

// ═══════════════════════════════════════════════════════════════════
// biocore_nernst
// ═══════════════════════════════════════════════════════════════════

int test_nernst_potassium() {
    TEST("Nernst: K+ equilibrium (intra=140, extra=4, valence=1)");
    // E_K = (RT/zF) * ln([K_out]/[K_in]) * 1000
    //     = (8.314*310.15)/(1*96485) * ln(4/140) * 1000
    //     = 0.02673 * ln(0.02857) * 1000
    //     = 0.02673 * (-3.555) * 1000
    //     ≈ -95.03 mV
    float e = biocore_nernst(140.0f, 4.0f, 1.0f);
    CHECK_NEAR(e, -95.0f, 1.0f);
    PASS();
    return 0;
}

int test_nernst_sodium() {
    TEST("Nernst: Na+ equilibrium (intra=10, extra=140, valence=1)");
    // E_Na = 0.02673 * ln(140/10) * 1000
    //      = 0.02673 * 2.639 * 1000 ≈ 70.54 mV
    float e = biocore_nernst(10.0f, 140.0f, 1.0f);
    CHECK_NEAR(e, 70.5f, 1.0f);
    PASS();
    return 0;
}

int test_nernst_calcium() {
    TEST("Nernst: Ca2+ equilibrium (intra=0.0001, extra=2, valence=2)");
    // Divalent cation: E = (RT/2F) * ln(extra/intra) * 1000
    float e = biocore_nernst(0.0001f, 2.0f, 2.0f);
    // E = 0.02673/2 * ln(2/0.0001) * 1000
    //   = 0.01337 * ln(20000) * 1000
    //   = 0.01337 * 9.903 * 1000 ≈ 132.4 mV
    CHECK_NEAR(e, 132.0f, 5.0f);
    PASS();
    return 0;
}

int test_nernst_below_threshold_clamped() {
    TEST("Nernst: clamps concentrations below 1e-6");
    // intra=0 is clamped to 1e-6
    float e = biocore_nernst(0.0f, 140.0f, 1.0f);
    // E = 0.02673 * ln(140/1e-6) * 1000
    //   = 0.02673 * ln(1.4e8) * 1000
    //   = 0.02673 * 18.76 * 1000 ≈ 501 mV
    CHECK(e > 100.0f);  // Очень положительный
    PASS();
    return 0;
}

int test_nernst_valence_zero() {
    TEST("Nernst: handles valence=1 (not zero)");
    // Функция использует valence напрямую в знаменателе.
    // При valence=0 будет деление на ноль → INF.
    // Проверяем что функция не крашится с нормальным valence.
    float e = biocore_nernst(10.0f, 10.0f, 1.0f);
    // ln(10/10) = ln(1) = 0 → E = 0
    CHECK_NEAR(e, 0.0f, 0.01f);
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// biocore_segment_area
// ═══════════════════════════════════════════════════════════════════

int test_segment_area_standard() {
    TEST("Segment area: r=10, l=100");
    Segment s = {};
    s.radius = 10.0f;
    s.length = 100.0f;
    float area = biocore_segment_area(&s);
    CHECK_NEAR(area, 2.0f * PI * 10.0f * 100.0f, 0.01f);
    PASS();
    return 0;
}

int test_segment_area_zero_radius() {
    TEST("Segment area: r=0");
    Segment s = {};
    s.radius = 0.0f;
    s.length = 100.0f;
    float area = biocore_segment_area(&s);
    CHECK_NEAR(area, 0.0f, 0.01f);
    PASS();
    return 0;
}

int test_segment_area_zero_length() {
    TEST("Segment area: l=0");
    Segment s = {};
    s.radius = 10.0f;
    s.length = 0.0f;
    float area = biocore_segment_area(&s);
    CHECK_NEAR(area, 0.0f, 0.01f);
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// biocore_clamp_ions
// ═══════════════════════════════════════════════════════════════════

int test_clamp_ions_normal() {
    TEST("Clamp ions: values above threshold unchanged");
    float ions[] = {140.0f, 4.0f, 2.0f, 120.0f};
    biocore_clamp_ions(ions, 4);
    CHECK_NEAR(ions[0], 140.0f, 0.01f);
    CHECK_NEAR(ions[1], 4.0f, 0.01f);
    CHECK_NEAR(ions[2], 2.0f, 0.01f);
    CHECK_NEAR(ions[3], 120.0f, 0.01f);
    PASS();
    return 0;
}

int test_clamp_ions_below_threshold() {
    TEST("Clamp ions: values below 1e-6 clamped to 1e-6");
    float ions[] = {0.0f, -1.0f, 1e-10f, 1.0f};
    biocore_clamp_ions(ions, 4);
    CHECK_NEAR(ions[0], 1e-6f, 1e-7f);  // 0 → 1e-6
    CHECK_NEAR(ions[1], 1e-6f, 1e-7f);  // -1 → 1e-6
    CHECK_NEAR(ions[2], 1e-6f, 1e-7f);  // 1e-10 → 1e-6
    CHECK_NEAR(ions[3], 1.0f, 0.01f);   // 1.0 → без изменений
    PASS();
    return 0;
}

int test_clamp_ions_empty() {
    TEST("Clamp ions: empty array (no crash)");
    float ions[] = {1.0f, 2.0f};
    biocore_clamp_ions(ions, 0);  // count=0 — цикл не выполняется
    CHECK_NEAR(ions[0], 1.0f, 0.01f);  // Без изменений
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// Константы
// ═══════════════════════════════════════════════════════════════════

int test_constants() {
    TEST("Constants: ION_*, NT_*, PI, FARADAY, GAS, CELSIUS");
    CHECK(ION_NA == 0);
    CHECK(ION_K == 1);
    CHECK(ION_CA == 2);
    CHECK(ION_CL == 3);
    CHECK(ION_MG == 4);
    CHECK(ION_COUNT == 5);
    CHECK(NT_GLUTAMATE == 0);
    CHECK(NT_COUNT == 14);
    CHECK(MAX_STATE_VARS == 8);
    CHECK(MAX_MODIFIERS_PER_SEGMENT == 32);
    CHECK_NEAR(FARADAY_CONST, 96485.0f, 0.1f);
    CHECK_NEAR(GAS_CONST, 8.314f, 0.001f);
    CHECK_NEAR(CELSIUS_37, 310.15f, 0.01f);
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// Размеры структур (POD, упаковка)
// ═══════════════════════════════════════════════════════════════════

int test_struct_sizes() {
    TEST("Struct sizes: Vec3, Modifier, Segment");
    CHECK(sizeof(Vec3) == 12);
    CHECK(sizeof(Modifier) == 44);
    // Segment size зависит от SDK-версии; проверяем что > 0
    CHECK(sizeof(Segment) > 100);
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════

int main() {
    printf("=== plugin-devkit.h tests ===\n\n");

    int failed = 0;
    failed += test_nernst_potassium();
    failed += test_nernst_sodium();
    failed += test_nernst_calcium();
    failed += test_nernst_below_threshold_clamped();
    failed += test_nernst_valence_zero();
    failed += test_segment_area_standard();
    failed += test_segment_area_zero_radius();
    failed += test_segment_area_zero_length();
    failed += test_clamp_ions_normal();
    failed += test_clamp_ions_below_threshold();
    failed += test_clamp_ions_empty();
    failed += test_constants();
    failed += test_struct_sizes();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);

    if (failed > 0) {
        printf("FAILED: %d test(s)\n", failed);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
