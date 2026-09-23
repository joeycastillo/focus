#include "test_harness.hpp"
#include "Locale.hpp"

#include <cstring>
#include <fstream>
#include <filesystem>

using namespace focus;

TEST(locale_from_memory_resolves_strings) {
    static const char blob[] = "greeting=Hello\ncount=You have {0}\n";
    Locale* loc = Locale::fromMemory("t_basic", (const uint8_t*)blob, sizeof(blob) - 1);
    ASSERT_TRUE(loc != nullptr);
    ASSERT_TRUE(loc->isValid());
    ASSERT_STREQ(loc->getString("greeting"), "Hello");
    ASSERT_STREQ(loc->getString("count"), "You have {0}");
    ASSERT_STREQ(loc->getIdentifier(), "t_basic");
}

TEST(locale_from_memory_reachable_via_withIdentifier) {
    static const char blob[] = "a=b\n";
    Locale* loc = Locale::fromMemory("t_cache", (const uint8_t*)blob, sizeof(blob) - 1);
    ASSERT_TRUE(loc != nullptr);
    ASSERT_EQ(Locale::withIdentifier("t_cache"), loc);
}

TEST(locale_from_memory_raw_string_with_nul_and_blank_lines) {
    // Raw literal: leading newline (blank first line) and, because we pass the full
    // sizeof, a trailing NUL after the final newline. Both must be tolerated.
    static const char blob[] = R"(
title=Music
empty=No music found in {0}
)";
    Locale* loc = Locale::fromMemory("t_raw", (const uint8_t*)blob, sizeof(blob));
    ASSERT_TRUE(loc != nullptr);
    ASSERT_STREQ(loc->getString("title"), "Music");
    ASSERT_STREQ(loc->getString("empty"), "No music found in {0}");
}

TEST(locale_from_memory_empty_returns_null) {
    static const char blob[] = "# just a comment, no keys\n";
    Locale* loc = Locale::fromMemory("t_empty", (const uint8_t*)blob, sizeof(blob) - 1);
    ASSERT_TRUE(loc == nullptr);
}

TEST(locale_from_file_still_works_after_refactor) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path();
    fs::path path = dir / "focus_t_file.strings";
    {
        std::ofstream out(path);
        out << "hello=World\nescaped=a\\nb\n";
    }
    Locale::clearSearchPaths();
    Locale::addLocaleSearchPath(dir.string());
    Locale* loc = Locale::withIdentifier("focus_t_file");
    ASSERT_TRUE(loc != nullptr);
    ASSERT_STREQ(loc->getString("hello"), "World");
    ASSERT_STREQ(loc->getString("escaped"), "a\nb");
    fs::remove(path);
    Locale::clearSearchPaths();
}

// Sets the current and default locales for one test, and clears both after.
struct LocaleScope {
    LocaleScope(Locale* current, Locale* fallback) {
        Locale::setCurrentLocale(current);
        Locale::setDefaultLocale(fallback);
    }
    ~LocaleScope() {
        Locale::setCurrentLocale(nullptr);
        Locale::setDefaultLocale(nullptr);
    }
};

static Locale* memoryLocale(const char* identifier, const char* strings) {
    return Locale::fromMemory(identifier, (const uint8_t*)strings, strlen(strings));
}

TEST(ls_checks_current_then_default_then_comment) {
    LocaleScope scope(memoryLocale("t_ls_cur", "a=Current\n"),
                      memoryLocale("t_ls_def", "a=Default\nb=DefaultB\n"));
    ASSERT_STREQ(_LS("a", "Comment"), "Current");
    ASSERT_STREQ(_LS("b", "Comment"), "DefaultB");
    ASSERT_STREQ(_LS("c", "Comment"), "Comment");
}

TEST(ls_without_locales_returns_comment) {
    LocaleScope scope(nullptr, nullptr);
    ASSERT_STREQ(_LS("a", "Comment"), "Comment");
}

TEST(lf_formats_whichever_string_wins) {
    LocaleScope scope(memoryLocale("t_lf_cur", "page=P {0}/{1}\n"),
                      memoryLocale("t_lf_def", "count=N {0}\n"));
    ASSERT_STREQ(_LF("page", "Page {0} of {1}", 1, 2), "P 1/2");
    ASSERT_STREQ(_LF("count", "Count {0}", 3), "N 3");
    ASSERT_STREQ(_LF("none", "Page {0}", 4), "Page 4");
}
