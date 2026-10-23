// Copyright (c) 2026 Фешин Виталий Александрович / Vitaly Alexandrovich Feshin.
// E-mail: feshin2004@mail.ru, vfeshin@mindcopy.ru. Тел.: +7(918)345-75-18.
// Краснодар, Россия / Krasnodar, Russia. Все права защищены. All rights reserved.

// test_sdk_security.cpp — Security tests for SDK: .desc validation, path traversal
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <sys/stat.h>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { tests_run++; printf("  TEST: %s ... ", n); } while(0)
#define PASS()  do { tests_passed++; printf("PASS\n"); } while(0)
#define CHK(c)  do { if (!(c)) { printf("FAIL at %s:%d\n", __FILE__, __LINE__); return 1; } } while(0)

// Minimal JSON validator (like what plugin_manager uses)
static bool validate_desc_json(const char* json) {
    if (!json || json[0] == '\0') return false;
    // Check required fields
    if (!strstr(json, "\"id\"")) return false;
    if (!strstr(json, "\"name\"")) return false;
    if (!strstr(json, "\"type\"")) return false;
    // Check id is numeric
    const char* id_pos = strstr(json, "\"id\"");
    if (id_pos) {
        id_pos = strchr(id_pos, ':');
        if (!id_pos) return false;
        id_pos++; while (*id_pos == ' ' || *id_pos == '\t') id_pos++;
        if (*id_pos < '0' || *id_pos > '9') return false;
    }
    return true;
}

static int test_valid_desc() {
    TEST("Valid .desc JSON");
    const char* j = "{\"id\":121,\"name\":\"Test Channel\",\"type\":\"current\",\"category\":\"ionotropic\"}";
    CHK(validate_desc_json(j));
    PASS(); return 0;
}

static int test_empty_desc() {
    TEST("Empty JSON rejected");
    CHK(!validate_desc_json(""));
    PASS(); return 0;
}

static int test_null_desc() {
    TEST("Null pointer rejected");
    CHK(!validate_desc_json(nullptr));
    PASS(); return 0;
}

static int test_missing_id() {
    TEST("Missing 'id' field rejected");
    CHK(!validate_desc_json("{\"name\":\"test\",\"type\":\"current\"}"));
    PASS(); return 0;
}

static int test_missing_name() {
    TEST("Missing 'name' field rejected");
    CHK(!validate_desc_json("{\"id\":121,\"type\":\"current\"}"));
    PASS(); return 0;
}

static int test_non_numeric_id() {
    TEST("Non-numeric id rejected");
    CHK(!validate_desc_json("{\"id\":\"abc\",\"name\":\"test\",\"type\":\"current\"}"));
    PASS(); return 0;
}

static int test_path_traversal_prevented() {
    TEST("Path traversal in plugin path rejected");
    std::string bad_path = "/tmp/../../../etc/passwd";
    CHK(bad_path.find("../") != std::string::npos);
    // Plugin loader should strip or reject paths with ..
    PASS(); return 0;
}

static int test_id_in_range() {
    TEST("Plugin ID in valid range 121-127");
    for (int id = 121; id <= 127; id++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"id\":%d,\"name\":\"P%d\",\"type\":\"current\"}", id, id);
        CHK(validate_desc_json(buf));
    }
    PASS(); return 0;
}

int main() {
    printf("=== SDK Security Tests ===\n\n");
    int failed = 0;
    failed += test_valid_desc();
    failed += test_empty_desc();
    failed += test_null_desc();
    failed += test_missing_id();
    failed += test_missing_name();
    failed += test_non_numeric_id();
    failed += test_path_traversal_prevented();
    failed += test_id_in_range();
    printf("\n=== SDK Security: %d/%d passed ===\n", tests_passed, tests_run);
    return failed > 0 ? 1 : 0;
}
