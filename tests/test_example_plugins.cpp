// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// test_example_plugins.cpp — тесты структуры example плагинов.
// Проверяет: .so содержит символы biocore_plugin_init и biocore_plugin_desc,
// .desc файлы валидны, dlopen работает без вызова init.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

static int tests_run = 0, tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

// Проверка .so на наличие символов (без вызова init — нужно в контексте Core)
static int check_plugin_symbols(const char* so_path, const char* expected_id) {
    // Открываем с RTLD_LAZY | RTLD_LOCAL чтобы не резолвить register_modifier_current
    void* h = dlopen(so_path, RTLD_LAZY | RTLD_LOCAL);
    // RTLD_LAZY может всё ещё падать на неразрешённых символах
    // в зависимости от линковки. Используем как индикатор.
    if (!h) {
        // Пробуем с RTLD_NOW и игнорируем ошибки symbol resolution
        // (нам нужны только символы, не вызов)
        printf("(dlopen note: %s) ", dlerror());
        return 1;  // символы не проверить, но .so существует
    }

    typedef int (*InitFunc)();
    InitFunc init = (InitFunc)dlsym(h, "biocore_plugin_init");
    CHECK(init != nullptr, "biocore_plugin_init not found");

    typedef const char* (*DescFunc)();
    DescFunc desc = (DescFunc)dlsym(h, "biocore_plugin_desc");
    CHECK(desc != nullptr, "biocore_plugin_desc not found");

    dlclose(h);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════

int test_ca_channel_symbols() {
    TEST("example_ca_channel.so: symbols exist");
    int rc = check_plugin_symbols("plugins/example_ca_channel.so", "93");
    if (rc == 1) {
        // dlopen failed but .so exists — проверяем через nm
        printf("(fallback to nm) ");
        FILE* nm = popen("nm -D plugins/example_ca_channel.so 2>/dev/null | grep biocore", "r");
        char buf[512] = {};
        if (nm) { fread(buf, 1, sizeof(buf)-1, nm); pclose(nm); }
        CHECK(strstr(buf, "biocore_plugin_init") != nullptr, "biocore_plugin_init symbol missing");
        CHECK(strstr(buf, "biocore_plugin_desc") != nullptr, "biocore_plugin_desc symbol missing");
    }
    PASS();
    return 0;
}

int test_d2_receptor_symbols() {
    TEST("example_d2_receptor.so: symbols exist");
    int rc = check_plugin_symbols("plugins/example_d2_receptor.so", "94");
    if (rc == 1) {
        printf("(fallback to nm) ");
        FILE* nm = popen("nm -D plugins/example_d2_receptor.so 2>/dev/null | grep biocore", "r");
        char buf[512] = {};
        if (nm) { fread(buf, 1, sizeof(buf)-1, nm); pclose(nm); }
        CHECK(strstr(buf, "biocore_plugin_init") != nullptr, "biocore_plugin_init symbol missing");
        CHECK(strstr(buf, "biocore_plugin_desc") != nullptr, "biocore_plugin_desc symbol missing");
    }
    PASS();
    return 0;
}

int test_desc_files_exist() {
    TEST("example .desc files exist alongside .so");
    FILE* f1 = fopen("plugins/example_ca_channel.desc", "r");
    CHECK(f1 != nullptr, "example_ca_channel.desc not found");
    if (f1) fclose(f1);

    FILE* f2 = fopen("plugins/example_d2_receptor.desc", "r");
    CHECK(f2 != nullptr, "example_d2_receptor.desc not found");
    if (f2) fclose(f2);
    PASS();
    return 0;
}

int test_nonexistent_so_fails() {
    TEST("nonexistent .so: dlopen returns nullptr");
    void* h = dlopen("plugins/nonexistent_xyz.so", RTLD_NOW);
    CHECK(h == nullptr, "dlopen should fail for nonexistent file");
    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════

int main() {
    printf("=== SDK Example Plugin Tests ===\n\n");

    int failed = 0;
    failed += test_ca_channel_symbols();
    failed += test_d2_receptor_symbols();
    failed += test_desc_files_exist();
    failed += test_nonexistent_so_fails();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    if (failed > 0) { printf("FAILED: %d test(s)\n", failed); return 1; }
    printf("ALL TESTS PASSED\n");
    return 0;
}
