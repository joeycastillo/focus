/*
 * Flag-off proof: the whole framework compiles with
 * FOCUS_HAS_PLURAL_RULES=0, every language uses the English plural rule,
 * and a registered rule still applies.
 *
 * Built only as the focus-test-noplurals executable, with its own main.
 */

#include "test_harness.hpp"
#include "Locale.hpp"

#include <cstring>
#include <initializer_list>

using namespace focus;
using P = PluralCategory;

static Locale* memoryLocale(const char* identifier, const char* strings) {
    return Locale::fromMemory(identifier, (const uint8_t*)strings, strlen(strings));
}

TEST(no_plurals_every_language_uses_the_english_rule) {
    for (const char* identifier : {"ar", "ru", "fr", "ja"}) {
        Locale* locale = memoryLocale(identifier, "x=y\n");
        ASSERT_TRUE(locale->pluralCategory(0) == P::Other);
        ASSERT_TRUE(locale->pluralCategory(1) == P::One);
        ASSERT_TRUE(locale->pluralCategory(2) == P::Other);
    }
}

TEST(no_plurals_registered_rule_still_applies) {
    Locale::setPluralRule("ru", [](uint64_t n) { return n % 10 == 2 ? P::Few : P::Other; });
    ASSERT_TRUE(memoryLocale("ru_RU", "x=y\n")->pluralCategory(22) == P::Few);
    Locale::setPluralRule("ru", nullptr);
    ASSERT_TRUE(memoryLocale("ru_RU", "x=y\n")->pluralCategory(22) == P::Other);
}

TEST(no_plurals_lp_uses_one_and_other) {
    Locale::setCurrentLocale(memoryLocale("ar_EG", "files.one=one\nfiles.two=two\nfiles.other={0} other\n"));
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 1), "one");
    ASSERT_STREQ(_LP("files", "{0} file", "{0} files", 2), "2 other");
    Locale::setCurrentLocale(nullptr);
}

int main() {
    return runAllTests();
}
