/*
 * Minimal unit test harness.
 *
 * Usage:
 *   TEST(test_name) {
 *       ASSERT_EQ(1 + 1, 2);
 *       ASSERT_TRUE(someCondition);
 *   }
 *
 * All TEST blocks auto-register. main() in main.cpp runs them all and
 * prints a summary. Exit code is 0 on all-pass, 1 on any failure.
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

struct TestCase {
    const char* name;
    std::function<void()> func;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(const char* name, std::function<void()> func) {
        testRegistry().push_back({name, std::move(func)});
    }
};

// Thrown on assertion failure to abort the current test (not the whole suite).
struct TestFailure {
    std::string message;
};

#define TEST(name) \
    static void test_##name(); \
    static TestRegistrar registrar_##name(#name, test_##name); \
    static void test_##name()

#define FAIL(msg) \
    do { \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg)); \
        throw TestFailure{msg}; \
    } while (0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #expr); \
            throw TestFailure{#expr}; \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_FALSE(%s)\n", __FILE__, __LINE__, #expr); \
            throw TestFailure{#expr}; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_EQ(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            throw TestFailure{#a " != " #b}; \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a == _b) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_NE(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            throw TestFailure{#a " == " #b}; \
        } \
    } while (0)

#define ASSERT_GT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a > _b)) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_GT(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            throw TestFailure{#a " <= " #b}; \
        } \
    } while (0)

#define ASSERT_GE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a >= _b)) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_GE(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            throw TestFailure{#a " < " #b}; \
        } \
    } while (0)

#define ASSERT_LT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a < _b)) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_LT(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            throw TestFailure{#a " >= " #b}; \
        } \
    } while (0)

#define ASSERT_STREQ(a, b) \
    do { \
        std::string _a = (a); std::string _b = (b); \
        if (_a != _b) { \
            fprintf(stderr, "  FAIL: %s:%d: ASSERT_STREQ(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            fprintf(stderr, "    got:      \"%s\"\n", _a.c_str()); \
            fprintf(stderr, "    expected: \"%s\"\n", _b.c_str()); \
            throw TestFailure{#a " != " #b}; \
        } \
    } while (0)

// Run all tests, return 0 on success, 1 on failure.
inline int runAllTests() {
    int passed = 0, failed = 0;
    for (auto& tc : testRegistry()) {
        fprintf(stderr, "  %s ... ", tc.name);
        try {
            tc.func();
            fprintf(stderr, "ok\n");
            passed++;
        } catch (const TestFailure&) {
            failed++;
        } catch (const std::exception& e) {
            fprintf(stderr, "  EXCEPTION: %s\n", e.what());
            failed++;
        }
    }
    fprintf(stderr, "\n%d passed, %d failed, %d total\n",
            passed, failed, passed + failed);
    return failed > 0 ? 1 : 0;
}
