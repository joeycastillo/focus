#include "test_harness.hpp"
#include "Locale.hpp"

#include <cstring>
#include <fstream>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <utility>
#include <vector>

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

// A placeholder catalog, for reading plural rules only.
static Locale* pluralLocale(const char* identifier) {
    static const char blob[] = "x=y\n";
    return Locale::fromMemory(identifier, (const uint8_t*)blob, sizeof(blob) - 1);
}

// Integer samples from CLDR 48.2 plurals.xml, one language per built-in rule plus English.
TEST(plural_rules_match_cldr_samples) {
    using P = PluralCategory;
    struct Case {
        const char* identifier;
        std::vector<std::pair<uint64_t, P>> samples;
    };
    const std::vector<Case> cases = {
        {"ja", {{0, P::Other}, {1, P::Other}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"hi", {{0, P::One}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"es", {{0, P::Other}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Many}}},
        {"fr", {{0, P::One}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Many}}},
        {"ru", {{0, P::Many}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Many}, {6, P::Many}, {7, P::Many}, {8, P::Many}, {9, P::Many}, {10, P::Many}, {11, P::Many}, {12, P::Many}, {13, P::Many}, {14, P::Many}, {15, P::Many}, {16, P::Many}, {17, P::Many}, {18, P::Many}, {19, P::Many}, {21, P::One}, {22, P::Few}, {23, P::Few}, {24, P::Few}, {100, P::Many}, {101, P::One}, {102, P::Few}, {1000, P::Many}, {1001, P::One}, {1002, P::Few}, {10000, P::Many}, {100000, P::Many}, {1000000, P::Many}}},
        {"pl", {{0, P::Many}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Many}, {6, P::Many}, {7, P::Many}, {8, P::Many}, {9, P::Many}, {10, P::Many}, {11, P::Many}, {12, P::Many}, {13, P::Many}, {14, P::Many}, {15, P::Many}, {16, P::Many}, {17, P::Many}, {18, P::Many}, {19, P::Many}, {22, P::Few}, {23, P::Few}, {24, P::Few}, {100, P::Many}, {102, P::Few}, {1000, P::Many}, {1002, P::Few}, {10000, P::Many}, {100000, P::Many}, {1000000, P::Many}}},
        {"cs", {{0, P::Other}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {18, P::Other}, {19, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"hr", {{0, P::Other}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {18, P::Other}, {19, P::Other}, {21, P::One}, {22, P::Few}, {23, P::Few}, {24, P::Few}, {100, P::Other}, {101, P::One}, {102, P::Few}, {1000, P::Other}, {1001, P::One}, {1002, P::Few}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"lt", {{0, P::Other}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Few}, {6, P::Few}, {7, P::Few}, {8, P::Few}, {9, P::Few}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {18, P::Other}, {19, P::Other}, {20, P::Other}, {21, P::One}, {22, P::Few}, {23, P::Few}, {24, P::Few}, {100, P::Other}, {101, P::One}, {102, P::Few}, {1000, P::Other}, {1001, P::One}, {1002, P::Few}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"lv", {{0, P::Zero}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Zero}, {11, P::Zero}, {12, P::Zero}, {13, P::Zero}, {14, P::Zero}, {15, P::Zero}, {16, P::Zero}, {17, P::Zero}, {18, P::Zero}, {19, P::Zero}, {20, P::Zero}, {21, P::One}, {22, P::Other}, {23, P::Other}, {24, P::Other}, {100, P::Zero}, {101, P::One}, {102, P::Other}, {1000, P::Zero}, {1001, P::One}, {1002, P::Other}, {10000, P::Zero}, {100000, P::Zero}, {1000000, P::Zero}}},
        {"ar", {{0, P::Zero}, {1, P::One}, {2, P::Two}, {3, P::Few}, {4, P::Few}, {5, P::Few}, {6, P::Few}, {7, P::Few}, {8, P::Few}, {9, P::Few}, {10, P::Few}, {11, P::Many}, {12, P::Many}, {13, P::Many}, {14, P::Many}, {15, P::Many}, {16, P::Many}, {17, P::Many}, {18, P::Many}, {19, P::Many}, {20, P::Many}, {21, P::Many}, {22, P::Many}, {23, P::Many}, {24, P::Many}, {100, P::Other}, {101, P::Other}, {102, P::Other}, {103, P::Few}, {104, P::Few}, {105, P::Few}, {106, P::Few}, {107, P::Few}, {108, P::Few}, {109, P::Few}, {110, P::Few}, {111, P::Many}, {200, P::Other}, {201, P::Other}, {202, P::Other}, {300, P::Other}, {301, P::Other}, {302, P::Other}, {400, P::Other}, {401, P::Other}, {402, P::Other}, {500, P::Other}, {501, P::Other}, {502, P::Other}, {600, P::Other}, {1000, P::Other}, {1003, P::Few}, {1011, P::Many}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"he", {{0, P::Other}, {1, P::One}, {2, P::Two}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"sl", {{0, P::Other}, {1, P::One}, {2, P::Two}, {3, P::Few}, {4, P::Few}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {17, P::Other}, {18, P::Other}, {19, P::Other}, {100, P::Other}, {101, P::One}, {102, P::Two}, {103, P::Few}, {104, P::Few}, {201, P::One}, {202, P::Two}, {203, P::Few}, {204, P::Few}, {301, P::One}, {302, P::Two}, {303, P::Few}, {304, P::Few}, {401, P::One}, {402, P::Two}, {403, P::Few}, {404, P::Few}, {501, P::One}, {502, P::Two}, {503, P::Few}, {504, P::Few}, {601, P::One}, {602, P::Two}, {603, P::Few}, {604, P::Few}, {701, P::One}, {702, P::Two}, {703, P::Few}, {704, P::Few}, {1000, P::Other}, {1001, P::One}, {1002, P::Two}, {1003, P::Few}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"ro", {{0, P::Few}, {1, P::One}, {2, P::Few}, {3, P::Few}, {4, P::Few}, {5, P::Few}, {6, P::Few}, {7, P::Few}, {8, P::Few}, {9, P::Few}, {10, P::Few}, {11, P::Few}, {12, P::Few}, {13, P::Few}, {14, P::Few}, {15, P::Few}, {16, P::Few}, {20, P::Other}, {21, P::Other}, {22, P::Other}, {23, P::Other}, {24, P::Other}, {100, P::Other}, {101, P::Few}, {1000, P::Other}, {1001, P::Few}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"is", {{0, P::Other}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {21, P::One}, {100, P::Other}, {101, P::One}, {1000, P::Other}, {1001, P::One}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
        {"en", {{0, P::Other}, {1, P::One}, {2, P::Other}, {3, P::Other}, {4, P::Other}, {5, P::Other}, {6, P::Other}, {7, P::Other}, {8, P::Other}, {9, P::Other}, {10, P::Other}, {11, P::Other}, {12, P::Other}, {13, P::Other}, {14, P::Other}, {15, P::Other}, {16, P::Other}, {100, P::Other}, {1000, P::Other}, {10000, P::Other}, {100000, P::Other}, {1000000, P::Other}}},
    };
    for (const auto& c : cases) {
        Locale* locale = pluralLocale(c.identifier);
        ASSERT_TRUE(locale != nullptr);
        for (const auto& [n, expected] : c.samples) {
            if (locale->pluralCategory((int64_t)n) != expected) {
                fprintf(stderr, "  %s at %llu\n", c.identifier, (unsigned long long)n);
                FAIL("plural category differs from CLDR");
            }
        }
    }
}

TEST(plural_rule_full_identifier_before_language) {
    using P = PluralCategory;
    ASSERT_TRUE(pluralLocale("pt")->pluralCategory(0) == P::One);
    ASSERT_TRUE(pluralLocale("pt_BR")->pluralCategory(0) == P::One);
    ASSERT_TRUE(pluralLocale("pt_PT")->pluralCategory(0) == P::Other);
    ASSERT_TRUE(pluralLocale("pt-PT")->pluralCategory(0) == P::Other);
    ASSERT_TRUE(pluralLocale("kok_Latn")->pluralCategory(0) == P::One);
}

TEST(plural_rule_language_part_is_not_truncated) {
    using P = PluralCategory;
    ASSERT_TRUE(pluralLocale("hsb")->pluralCategory(2) == P::Two);
    ASSERT_TRUE(pluralLocale("hsb_DE")->pluralCategory(2) == P::Two);
    ASSERT_TRUE(pluralLocale("ars")->pluralCategory(0) == P::Zero);
}

TEST(plural_rule_unknown_language_uses_default) {
    using P = PluralCategory;
    ASSERT_TRUE(pluralLocale("xx")->pluralCategory(0) == P::Other);
    ASSERT_TRUE(pluralLocale("xx")->pluralCategory(1) == P::One);
    ASSERT_TRUE(pluralLocale("xx")->pluralCategory(2) == P::Other);
    ASSERT_TRUE(pluralLocale("cy")->pluralCategory(2) == P::Other);
}

TEST(plural_rule_negative_count_uses_absolute_value) {
    using P = PluralCategory;
    Locale* ru = pluralLocale("ru");
    ASSERT_TRUE(ru->pluralCategory(-1) == P::One);
    ASSERT_TRUE(ru->pluralCategory(-2) == P::Few);
    ASSERT_TRUE(ru->pluralCategory(-5) == P::Many);
    ASSERT_TRUE(ru->pluralCategory(std::numeric_limits<int64_t>::min()) == P::Many);
    ASSERT_TRUE(pluralLocale("xx")->pluralCategory(-1) == P::One);
}

TEST(registered_plural_rule_applies_until_removed) {
    using P = PluralCategory;
    Locale::setPluralRule("xq", [](uint64_t n) { return n == 2 ? P::Two : P::Other; });
    ASSERT_TRUE(pluralLocale("xq")->pluralCategory(2) == P::Two);
    ASSERT_TRUE(pluralLocale("xq_ZZ")->pluralCategory(2) == P::Two);
    ASSERT_TRUE(pluralLocale("xq")->pluralCategory(1) == P::Other);
    Locale::setPluralRule("xq", nullptr);
    ASSERT_TRUE(pluralLocale("xq")->pluralCategory(1) == P::One);
}

TEST(registered_plural_rule_overrides_builtin) {
    using P = PluralCategory;
    Locale::setPluralRule("ar", [](uint64_t) { return P::Other; });
    ASSERT_TRUE(pluralLocale("ar")->pluralCategory(1) == P::Other);
    Locale::setPluralRule("ar", nullptr);
    ASSERT_TRUE(pluralLocale("ar")->pluralCategory(1) == P::One);

    Locale::setPluralRule("pt-PT", [](uint64_t) { return P::Many; });
    ASSERT_TRUE(pluralLocale("pt_PT")->pluralCategory(1) == P::Many);
    ASSERT_TRUE(pluralLocale("pt")->pluralCategory(1) == P::One);
    Locale::setPluralRule("pt_PT", nullptr);
    ASSERT_TRUE(pluralLocale("pt_PT")->pluralCategory(1) == P::One);
}

TEST(registered_plural_rule_precedence) {
    using P = PluralCategory;
    Locale::setPluralRule("xr", [](uint64_t) { return P::Few; });
    Locale::setPluralRule("xr_ZZ", [](uint64_t) { return P::Many; });
    ASSERT_TRUE(pluralLocale("xr_ZZ")->pluralCategory(1) == P::Many);
    ASSERT_TRUE(pluralLocale("xr_YY")->pluralCategory(1) == P::Few);
    Locale::setPluralRule("xr", nullptr);
    Locale::setPluralRule("xr_ZZ", nullptr);

    Locale::setPluralRule("pt", [](uint64_t) { return P::Few; });
    ASSERT_TRUE(pluralLocale("pt_PT")->pluralCategory(1) == P::Few);
    Locale::setPluralRule("pt", nullptr);
    ASSERT_TRUE(pluralLocale("pt_PT")->pluralCategory(1) == P::One);
}

TEST(lp_without_catalog_uses_english_forms) {
    LocaleScope scope(nullptr, nullptr);
    ASSERT_STREQ(_LP("items", "{0} item", "{0} items", 1), "1 item");
    ASSERT_STREQ(_LP("items", "{0} item", "{0} items", 2), "2 items");
    ASSERT_STREQ(_LP("items", "{0} item", "{0} items", 0), "0 items");
    ASSERT_STREQ(_LP("items", "{0} item", "{0} items", -1), "-1 item");
}

TEST(lp_uses_the_category_from_the_locale_rule) {
    LocaleScope scope(memoryLocale("ar_T1",
        "files.zero=zero\nfiles.one=one\nfiles.two=two\n"
        "files.few={0} few\nfiles.many={0} many\nfiles.other={0} other\n"), nullptr);
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 0), "zero");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 1), "one");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 2), "two");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 3), "3 few");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 11), "11 many");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 100), "100 other");
}

TEST(lp_missing_category_falls_back_to_other) {
    LocaleScope scope(memoryLocale("ru_T1", "files.one={0} one\nfiles.other={0} other\n"), nullptr);
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 5), "5 other");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 21), "21 one");
}

TEST(lp_checks_current_locale_before_default) {
    LocaleScope scope(memoryLocale("ru_T2", "files.other=RU {0}\n"),
                      memoryLocale("ar_T2", "files.one=AR one\nbooks.two=AR two\n"));
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 1), "RU 1");
    ASSERT_STREQ(_LP("books", "{0} book", "{0} books", 2), "AR two");
    ASSERT_STREQ(_LP("books", "{0} book", "{0} books", 3), "3 books");
}

TEST(lp_zero_key_is_only_a_grammatical_category) {
    LocaleScope scope(memoryLocale("en_T1", "files.zero=none\nfiles.other={0} files\n"), nullptr);
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 0), "0 files");
}

TEST(lp_negative_count_keeps_its_sign) {
    LocaleScope scope(memoryLocale("ru_T3",
        "files.one={0} one\nfiles.few={0} few\nfiles.many={0} many\n"), nullptr);
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", -21), "-21 one");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", -2), "-2 few");
}

// Writes catalogs to the temp directory and searches only there; removes them after.
struct CatalogDir {
    std::vector<std::filesystem::path> paths;
    explicit CatalogDir(std::initializer_list<std::pair<const char*, const char*>> catalogs) {
        auto dir = std::filesystem::temp_directory_path();
        Locale::clearSearchPaths();
        Locale::addLocaleSearchPath(dir.string());
        for (const auto& [identifier, contents] : catalogs) {
            paths.push_back(dir / (std::string(identifier) + ".strings"));
            std::ofstream out(paths.back());
            out << contents;
        }
    }
    ~CatalogDir() {
        for (const auto& path : paths) std::filesystem::remove(path);
        Locale::clearSearchPaths();
    }
};

TEST(locale_parent_strings_fill_in_under_the_child) {
    CatalogDir dir({{"fp1", "a=Parent A\nb=Parent B\n"},
                    {"fp1_GB", "@parent=fp1\n@future=x\nb=Child B\n"}});
    Locale* locale = Locale::withIdentifier("fp1_GB");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "Parent A");
    ASSERT_STREQ(locale->getString("b"), "Child B");
    ASSERT_STREQ(locale->getString("@parent"), "");
    ASSERT_STREQ(locale->getString("@future"), "");
}

TEST(locale_parent_is_not_cached) {
    CatalogDir dir({{"fp2", "a=Parent A\n"}, {"fp2_GB", "@parent=fp2\nb=Child B\n"}});
    ASSERT_STREQ(Locale::withIdentifier("fp2_GB")->getString("a"), "Parent A");
    std::filesystem::remove(dir.paths[0]);
    ASSERT_TRUE(Locale::withIdentifier("fp2") == nullptr);
}

TEST(locale_parent_chain_resolves_three_levels) {
    CatalogDir dir({{"fp3", "a=A\n"},
                    {"fp3_001", "@parent=fp3\nb=B\n"},
                    {"fp3_AU", "@parent=fp3_001\nc=C\n"}});
    Locale* locale = Locale::withIdentifier("fp3_AU");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "A");
    ASSERT_STREQ(locale->getString("b"), "B");
    ASSERT_STREQ(locale->getString("c"), "C");
}

TEST(locale_missing_parent_leaves_child_standalone) {
    CatalogDir dir({{"fp4_GB", "@parent=fp4_missing\nb=Child B\n"}});
    Locale* locale = Locale::withIdentifier("fp4_GB");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("b"), "Child B");
}

TEST(locale_parent_only_catalog_loads) {
    CatalogDir dir({{"fp5", "a=A\n"}, {"fp5_CA", "@parent=fp5\n"}});
    Locale* locale = Locale::withIdentifier("fp5_CA");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "A");
}

TEST(locale_parent_cycle_terminates) {
    CatalogDir dir({{"fp6a", "@parent=fp6b\na=A\n"}, {"fp6b", "@parent=fp6a\nb=B\n"}});
    Locale* locale = Locale::withIdentifier("fp6a");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "A");
    ASSERT_STREQ(locale->getString("b"), "B");
}

TEST(locale_blob_merges_parent_loaded_first) {
    Locale::clearSearchPaths();
    memoryLocale("mp_en", "a=Parent A\nb=Parent B\n");
    Locale* locale = memoryLocale("mp_en_GB", "@parent=mp_en\nb=Child B\n");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "Parent A");
    ASSERT_STREQ(locale->getString("b"), "Child B");
}

TEST(locale_blob_loaded_before_parent_stands_alone) {
    Locale::clearSearchPaths();
    Locale* locale = memoryLocale("mc_en_GB", "@parent=mc_en\nb=Child B\n");
    memoryLocale("mc_en", "a=Parent A\n");
    ASSERT_TRUE(locale != nullptr);
    ASSERT_STREQ(locale->getString("a"), "");
    ASSERT_STREQ(locale->getString("b"), "Child B");
}

TEST(lp_uses_parent_forms_the_child_does_not_override) {
    Locale::clearSearchPaths();
    memoryLocale("pp_en", "items.one={0} item\nitems.other={0} items\n");
    LocaleScope scope(memoryLocale("pp_en_GB", "@parent=pp_en\nitems.other={0} things\n"), nullptr);
    ASSERT_STREQ(_LP("items", "{0} x", "{0} xs", 1), "1 item");
    ASSERT_STREQ(_LP("items", "{0} x", "{0} xs", 2), "2 things");
}

TEST(clear_cache_frees_a_blob_parent) {
    Locale::clearSearchPaths();
    memoryLocale("cc_en", "a=Parent A\n");
    LocaleScope scope(memoryLocale("cc_en_GB", "@parent=cc_en\nb=Child B\n"), nullptr);
    Locale::clearCache();
    ASSERT_TRUE(Locale::withIdentifier("cc_en") == nullptr);
    ASSERT_STREQ(Locale::withIdentifier("cc_en_GB")->getString("a"), "Parent A");
}
