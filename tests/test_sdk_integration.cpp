// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// test_sdk_integration.cpp — Integration: полный цикл плагина
// (build .so → dlopen → dlsym → call → dlclose → cleanup)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { tests_run++; printf("  TEST: %s ... ", n); } while(0)
#define PASS()  do { tests_passed++; printf("PASS\n"); } while(0)
#define CHK(c)  do { if (!(c)) { printf("FAIL at %s:%d\n", __FILE__, __LINE__); return 1; } } while(0)

// Compile a minimal plugin .so from source
static bool compile_plugin(const char* src, const char* out_so, const char* sdk_include, const char* core_include) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "g++ -std=c++17 -fPIC -shared -O2 -I%s -I%s -o %s %s 2>/dev/null",
        sdk_include, core_include, out_so, src);
    return system(cmd) == 0;
}

static int test_full_plugin_lifecycle() {
    TEST("Full plugin lifecycle: compile → load → symbols → unload");
    const char* sdk_inc = "../include";
    const char* core_inc = "../BioNeuronCore/include";

    // Minimal plugin source
    const char* src = "/tmp/test_minimal_plugin.cpp";
    const char* so  = "/tmp/test_minimal_plugin.so";
    FILE* f = fopen(src, "w");
    fprintf(f, "%s\n",
        "#include \"plugin-devkit.h\"\n"
        "extern \"C\" {\n"
        "int biocore_plugin_init() { return 1; }\n"
        "const char* biocore_plugin_desc() {\n"
        "  return \"{\\\"id\\\":121,\\\"name\\\":\\\"Minimal\\\",\\\"type\\\":\\\"current\\\"}\";\n"
        "}\n"
        "}");
    fclose(f);

    // Compile
    bool ok = compile_plugin(src, so, sdk_inc, core_inc);
    CHK(ok);
    CHK(access(so, F_OK) == 0);

    // dlopen
    void* h = dlopen(so, RTLD_NOW | RTLD_LOCAL);
    CHK(h != nullptr);

    // dlsym — init
    typedef int (*InitFunc)();
    InitFunc init = (InitFunc)dlsym(h, "biocore_plugin_init");
    CHK(init != nullptr);

    // dlsym — desc
    typedef const char* (*DescFunc)();
    DescFunc desc = (DescFunc)dlsym(h, "biocore_plugin_desc");
    CHK(desc != nullptr);

    // Call init
    int n = init();
    CHK(n > 0);

    // Call desc
    const char* d = desc();
    CHK(d != nullptr);
    CHK(strstr(d, "\"id\"") != nullptr);

    // dlclose
    int rc = dlclose(h);
    CHK(rc == 0);

    unlink(src); unlink(so);
    PASS(); return 0;
}

static int test_missing_plugin_graceful() {
    TEST("Missing plugin .so — graceful handling");
    void* h = dlopen("/tmp/nonexistent_plugin_xyz.so", RTLD_NOW | RTLD_LOCAL);
    CHK(h == nullptr);  // должен вернуть NULL, не краш
    PASS(); return 0;
}

static int test_corrupted_so_graceful() {
    TEST("Corrupted .so — graceful handling");
    FILE* f = fopen("/tmp/corrupted.so", "wb");
    const char garbage[] = "not an ELF file";
    fwrite(garbage, 1, sizeof(garbage), f);
    fclose(f);
    void* h = dlopen("/tmp/corrupted.so", RTLD_NOW | RTLD_LOCAL);
    CHK(h == nullptr);
    unlink("/tmp/corrupted.so");
    PASS(); return 0;
}

int main() {
    printf("=== SDK Integration Tests ===\n\n");
    int failed = 0;
    failed += test_full_plugin_lifecycle();
    failed += test_missing_plugin_graceful();
    failed += test_corrupted_so_graceful();
    printf("\n=== SDK Integration: %d/%d passed ===\n", tests_passed, tests_run);
    return failed > 0 ? 1 : 0;
}
