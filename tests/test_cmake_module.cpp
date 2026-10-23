// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// test_cmake_module.cpp — тесты biocore-plugin.cmake.
// Проверяет: biocore_add_plugin (сборка .so), biocore_gen_dispatch,
// biocore_find_sdk, обработку ошибок.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

static int tests_run = 0, tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int run_cmd(const char* cmd, char* out, int out_sz) {
    FILE* f = popen(cmd, "r");
    if (!f) return -1;
    if (out && out_sz > 0) {
        int n = (int)fread(out, 1, out_sz - 1, f);
        out[n] = 0;
    } else {
        char buf[256];
        while (fread(buf, 1, sizeof(buf), f) > 0) {}
    }
    return pclose(f);
}

// Возвращает абсолютный путь, разрешая ../ из cwd (SDK build/)
// Используется чтобы встроить правильные пути в CMakeLists.txt
static bool resolve_abs_path(const char* rel, char* out, int out_sz) {
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) return false;
    snprintf(out, out_sz, "%s/%s", cwd, rel);

    // Упрощённый resolve ".." — через realpath если доступен
    char* rp = realpath(out, nullptr);
    if (rp) {
        strncpy(out, rp, out_sz - 1);
        out[out_sz - 1] = 0;
        free(rp);
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════

int test_cmake_module_exists() {
    TEST("biocore-plugin.cmake exists and is readable");
    char sdk_abs[1024];
    resolve_abs_path("..", sdk_abs, sizeof(sdk_abs));
    char cmake_path[1100];
    snprintf(cmake_path, sizeof(cmake_path), "%s/cmake/biocore-plugin.cmake", sdk_abs);
    CHECK(file_exists(cmake_path), "biocore-plugin.cmake not found");
    PASS();
    return 0;
}

int test_generate_plugin_so() {
    TEST("biocore_add_plugin: builds .so from source + desc");

    // Абсолютные пути для встраивания в CMakeLists.txt
    char sdk_abs[1024], core_abs[1024];
    resolve_abs_path("..", sdk_abs, sizeof(sdk_abs));
    resolve_abs_path("../../BioNeuronCore", core_abs, sizeof(core_abs));

    const char* tmp = "/tmp/test_cmake_module_so";
    run_cmd("rm -rf /tmp/test_cmake_module_so /tmp/test_cmake_module_build", nullptr, 0);
    mkdir(tmp, 0755);
    mkdir("/tmp/test_cmake_module_build", 0755);

    FILE* f = fopen("/tmp/test_cmake_module_so/plugin_test.cpp", "w");
    fprintf(f, "#include <cstdio>\n");
    fprintf(f, "extern \"C\" int biocore_plugin_init() { return 42; }\n");
    fprintf(f, "extern \"C\" const char* biocore_plugin_desc() { return \"{}\"; }\n");
    fclose(f);

    f = fopen("/tmp/test_cmake_module_so/plugin_test.desc", "w");
    fprintf(f, "{\"id\":121,\"name\":\"test_cmake_plugin\",\"category\":\"ion_channel\"}\n");
    fclose(f);

    f = fopen("/tmp/test_cmake_module_so/CMakeLists.txt", "w");
    fprintf(f, "cmake_minimum_required(VERSION 3.16)\n");
    fprintf(f, "project(TestCmakePlugin)\n");
    fprintf(f, "set(BIOCORE_ROOT \"%s\")\n", core_abs);
    fprintf(f, "set(BIOCORE_SDK_DIR \"%s\")\n", sdk_abs);
    fprintf(f, "include(\"%s/cmake/biocore-plugin.cmake\")\n", sdk_abs);
    fprintf(f, "biocore_add_plugin(test_cmake_plugin plugin_test.cpp DESC plugin_test.desc)\n");
    fclose(f);

    (void)run_cmd("cmake /tmp/test_cmake_module_so -B /tmp/test_cmake_module_build 2>&1", nullptr, 0);
    (void)run_cmd("make -C /tmp/test_cmake_module_build -j$(nproc) 2>&1", nullptr, 0);

    CHECK(file_exists("/tmp/test_cmake_module_build/plugins/test_cmake_plugin.so"),
          ".so not built by biocore_add_plugin");

    void* h = dlopen("/tmp/test_cmake_module_build/plugins/test_cmake_plugin.so", RTLD_NOW | RTLD_LOCAL);
    CHECK(h != nullptr, "dlopen .so failed");

    typedef int (*InitFunc)();
    InitFunc init = (InitFunc)dlsym(h, "biocore_plugin_init");
    CHECK(init != nullptr, "biocore_plugin_init not found in built .so");

    typedef const char* (*DescFunc)();
    DescFunc desc = (DescFunc)dlsym(h, "biocore_plugin_desc");
    CHECK(desc != nullptr, "biocore_plugin_desc not found in built .so");

    CHECK(init() == 42, "biocore_plugin_init should return 42");

    dlclose(h);
    PASS();
    return 0;
}

int test_desc_copied() {
    TEST("biocore_add_plugin: .desc copied alongside .so");
    CHECK(file_exists("/tmp/test_cmake_module_build/plugins/test_cmake_plugin.desc"),
          ".desc not copied to output dir");
    PASS();
    return 0;
}

int test_gen_dispatch() {
    TEST("biocore_gen_dispatch: generates dispatch from .desc files");
    char sdk_abs[1024], core_abs[1024];
    resolve_abs_path("..", sdk_abs, sizeof(sdk_abs));
    resolve_abs_path("../../BioNeuronCore", core_abs, sizeof(core_abs));

    const char* tmp = "/tmp/test_cmake_module_dispatch_src";
    run_cmd("rm -rf /tmp/test_cmake_module_dispatch_src /tmp/test_cmake_module_dispatch", nullptr, 0);
    mkdir(tmp, 0755);
    mkdir("/tmp/test_cmake_module_dispatch", 0755);

    // Копируем .desc файл (нужен для генерации dispatch)
    FILE* df = fopen("/tmp/test_cmake_module_dispatch_src/plugin_test.desc", "w");
    fprintf(df, "{\"id\":121,\"name\":\"test_cmake_plugin\",\"category\":\"ion_channel\"}\n");
    fclose(df);

    FILE* f = fopen("/tmp/test_cmake_module_dispatch_src/CMakeLists.txt", "w");
    fprintf(f, "cmake_minimum_required(VERSION 3.16)\n");
    fprintf(f, "project(TestDispatchGen)\n");
    fprintf(f, "set(BIOCORE_ROOT \"%s\")\n", core_abs);
    fprintf(f, "set(BIOCORE_SDK_DIR \"%s\")\n", sdk_abs);
    fprintf(f, "include(\"%s/cmake/biocore-plugin.cmake\")\n", sdk_abs);
    fprintf(f, "biocore_gen_dispatch(PLUGINS_DIR \"%s\" OUTPUT \"${CMAKE_BINARY_DIR}/dispatch_generated_test.cpp\")\n",
            tmp);
    fclose(f);

    (void)run_cmd("cmake -S /tmp/test_cmake_module_dispatch_src -B /tmp/test_cmake_module_dispatch 2>&1", nullptr, 0);
    (void)run_cmd("make -C /tmp/test_cmake_module_dispatch gen_dispatch 2>&1", nullptr, 0);

    CHECK(file_exists("/tmp/test_cmake_module_dispatch/dispatch_generated_test.cpp"),
          "dispatch_generated_test.cpp not generated");

    char buf[4096] = {};
    FILE* gf = fopen("/tmp/test_cmake_module_dispatch/dispatch_generated_test.cpp", "r");
    CHECK(gf != nullptr, "cannot open generated dispatch file");
    (void)fread(buf, 1, sizeof(buf) - 1, gf);
    fclose(gf);
    CHECK(strstr(buf, "121") != nullptr,
          "generated dispatch doesn't reference plugin id 121");

    PASS();
    return 0;
}

int test_find_sdk() {
    TEST("biocore_find_sdk: detects SDK and sets variables");
    char sdk_abs[1024], core_abs[1024];
    resolve_abs_path("..", sdk_abs, sizeof(sdk_abs));
    resolve_abs_path("../../BioNeuronCore", core_abs, sizeof(core_abs));

    const char* tmp = "/tmp/test_cmake_module_find_src";
    run_cmd("rm -rf /tmp/test_cmake_module_find_src /tmp/test_cmake_module_find", nullptr, 0);
    mkdir(tmp, 0755);
    mkdir("/tmp/test_cmake_module_find", 0755);

    FILE* f = fopen("/tmp/test_cmake_module_find_src/CMakeLists.txt", "w");
    fprintf(f, "cmake_minimum_required(VERSION 3.16)\n");
    fprintf(f, "project(TestFindSdk)\n");
    fprintf(f, "set(BIOCORE_ROOT \"%s\")\n", core_abs);
    fprintf(f, "set(BIOCORE_SDK_DIR \"%s\")\n", sdk_abs);
    fprintf(f, "include(\"%s/cmake/biocore-plugin.cmake\")\n", sdk_abs);
    fprintf(f, "biocore_find_sdk()\n");
    fprintf(f, "if(NOT BIOCORE_SDK_FOUND)\n");
    fprintf(f, "  message(FATAL_ERROR \"SDK not found\")\n");
    fprintf(f, "endif()\n");
    fprintf(f, "message(STATUS \"SDK include: ${BIOCORE_SDK_INCLUDE_DIR}\")\n");
    fclose(f);

    char out[4096] = {};
    (void)run_cmd("cmake -S /tmp/test_cmake_module_find_src -B /tmp/test_cmake_module_find 2>&1", out, sizeof(out));

    CHECK(strstr(out, "BioNeuronSDK found") != nullptr || strstr(out, "SDK include") != nullptr,
          "biocore_find_sdk did not find SDK");

    PASS();
    return 0;
}

int test_missing_desc_no_error() {
    TEST("biocore_add_plugin: works without DESC");
    char sdk_abs[1024], core_abs[1024];
    resolve_abs_path("..", sdk_abs, sizeof(sdk_abs));
    resolve_abs_path("../../BioNeuronCore", core_abs, sizeof(core_abs));

    const char* tmp = "/tmp/test_cmake_module_no_desc";
    run_cmd("rm -rf /tmp/test_cmake_module_no_desc /tmp/test_cmake_module_no_desc_build", nullptr, 0);
    mkdir(tmp, 0755);
    mkdir("/tmp/test_cmake_module_no_desc_build", 0755);

    FILE* f = fopen("/tmp/test_cmake_module_no_desc/plugin_nodesc.cpp", "w");
    fprintf(f, "#include <cstdio>\n");
    fprintf(f, "extern \"C\" int biocore_plugin_init() { return 1; }\n");
    fprintf(f, "extern \"C\" const char* biocore_plugin_desc() { return \"{}\"; }\n");
    fclose(f);

    f = fopen("/tmp/test_cmake_module_no_desc/CMakeLists.txt", "w");
    fprintf(f, "cmake_minimum_required(VERSION 3.16)\n");
    fprintf(f, "project(TestNoDesc)\n");
    fprintf(f, "set(BIOCORE_ROOT \"%s\")\n", core_abs);
    fprintf(f, "set(BIOCORE_SDK_DIR \"%s\")\n", sdk_abs);
    fprintf(f, "include(\"%s/cmake/biocore-plugin.cmake\")\n", sdk_abs);
    fprintf(f, "biocore_add_plugin(test_nodesc plugin_nodesc.cpp)\n");
    fclose(f);

    (void)run_cmd("cmake /tmp/test_cmake_module_no_desc -B /tmp/test_cmake_module_no_desc_build 2>&1", nullptr, 0);
    (void)run_cmd("make -C /tmp/test_cmake_module_no_desc_build -j$(nproc) 2>&1", nullptr, 0);

    CHECK(file_exists("/tmp/test_cmake_module_no_desc_build/plugins/test_nodesc.so"),
          ".so without DESC should still build");
    CHECK(!file_exists("/tmp/test_cmake_module_no_desc_build/plugins/test_nodesc.desc"),
          ".desc should not exist when DESC not specified");

    PASS();
    return 0;
}

// ═══════════════════════════════════════════════════════════════════

int main() {
    printf("=== SDK cmake_module Tests ===\n\n");

    int failed = 0;
    failed += test_cmake_module_exists();
    failed += test_generate_plugin_so();
    failed += test_desc_copied();
    failed += test_gen_dispatch();
    failed += test_find_sdk();
    failed += test_missing_desc_no_error();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    if (failed > 0) { printf("FAILED: %d test(s)\n", failed); return 1; }
    printf("ALL TESTS PASSED\n");
    return 0;
}
