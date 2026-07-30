#include "test_harness.hpp"
#include "Locale.hpp"

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
