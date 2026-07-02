/*
 * Tests for Unicode case mappings, mirroring, and traits.
 */

#include "test_harness.hpp"
#include "UnicodeMappings.hpp"
#include "UnicodeTraits.hpp"

using namespace focus;

using namespace UnicodeMappings;

// --- Case conversion: ASCII ---

TEST(unicode_uppercase_ascii) {
    ASSERT_EQ(toUppercase('a'), (UNICODE_CODEPOINT)'A');
    ASSERT_EQ(toUppercase('z'), (UNICODE_CODEPOINT)'Z');
}

TEST(unicode_lowercase_ascii) {
    ASSERT_EQ(toLowercase('A'), (UNICODE_CODEPOINT)'a');
    ASSERT_EQ(toLowercase('Z'), (UNICODE_CODEPOINT)'z');
}

TEST(unicode_case_passthrough) {
    // Characters with no case mapping return themselves
    ASSERT_EQ(toUppercase('1'), (UNICODE_CODEPOINT)'1');
    ASSERT_EQ(toLowercase('!'), (UNICODE_CODEPOINT)'!');
    ASSERT_EQ(toUppercase(' '), (UNICODE_CODEPOINT)' ');
}

TEST(unicode_uppercase_already_upper) {
    ASSERT_EQ(toUppercase('A'), (UNICODE_CODEPOINT)'A');
}

TEST(unicode_lowercase_already_lower) {
    ASSERT_EQ(toLowercase('a'), (UNICODE_CODEPOINT)'a');
}

// --- Case conversion: Latin Extended ---

TEST(unicode_uppercase_latin_extended) {
    // é (U+00E9) → É (U+00C9)
    ASSERT_EQ(toUppercase(0x00E9), (UNICODE_CODEPOINT)0x00C9);
}

TEST(unicode_lowercase_latin_extended) {
    // É (U+00C9) → é (U+00E9)
    ASSERT_EQ(toLowercase(0x00C9), (UNICODE_CODEPOINT)0x00E9);
}

TEST(unicode_uppercase_german_sharp_s) {
    // ß (U+00DF) → uppercase is ẞ (U+1E9E) in Unicode 5.1+
    // If the table doesn't have it, it returns ß (passthrough)
    UNICODE_CODEPOINT result = toUppercase(0x00DF);
    // Accept either passthrough or U+1E9E
    ASSERT_TRUE(result == 0x00DF || result == 0x1E9E);
}

// --- Case conversion: Greek ---

TEST(unicode_uppercase_greek) {
    // α (U+03B1) → Α (U+0391)
    ASSERT_EQ(toUppercase(0x03B1), (UNICODE_CODEPOINT)0x0391);
}

TEST(unicode_lowercase_greek) {
    // Ω (U+03A9) → ω (U+03C9)
    ASSERT_EQ(toLowercase(0x03A9), (UNICODE_CODEPOINT)0x03C9);
}

// --- Case conversion: Cyrillic ---

TEST(unicode_uppercase_cyrillic) {
    // а (U+0430) → А (U+0410)
    ASSERT_EQ(toUppercase(0x0430), (UNICODE_CODEPOINT)0x0410);
}

TEST(unicode_lowercase_cyrillic) {
    // Я (U+042F) → я (U+044F)
    ASSERT_EQ(toLowercase(0x042F), (UNICODE_CODEPOINT)0x044F);
}

// --- Titlecase ---

TEST(unicode_titlecase_ascii) {
    // For most characters, titlecase == uppercase
    ASSERT_EQ(toTitlecase('a'), (UNICODE_CODEPOINT)'A');
}

TEST(unicode_titlecase_passthrough) {
    // Already titlecase / no mapping
    ASSERT_EQ(toTitlecase('1'), (UNICODE_CODEPOINT)'1');
}

// --- Buffer conversion ---

TEST(unicode_uppercase_buffer) {
    UNICODE_CODEPOINT buf[] = {'h', 'e', 'l', 'l', 'o'};
    toUppercase(buf, 5);
    ASSERT_EQ(buf[0], (UNICODE_CODEPOINT)'H');
    ASSERT_EQ(buf[1], (UNICODE_CODEPOINT)'E');
    ASSERT_EQ(buf[2], (UNICODE_CODEPOINT)'L');
    ASSERT_EQ(buf[3], (UNICODE_CODEPOINT)'L');
    ASSERT_EQ(buf[4], (UNICODE_CODEPOINT)'O');
}

TEST(unicode_lowercase_buffer) {
    UNICODE_CODEPOINT buf[] = {'H', 'E', 'L', 'L', 'O'};
    toLowercase(buf, 5);
    ASSERT_EQ(buf[0], (UNICODE_CODEPOINT)'h');
    ASSERT_EQ(buf[1], (UNICODE_CODEPOINT)'e');
    ASSERT_EQ(buf[2], (UNICODE_CODEPOINT)'l');
    ASSERT_EQ(buf[3], (UNICODE_CODEPOINT)'l');
    ASSERT_EQ(buf[4], (UNICODE_CODEPOINT)'o');
}

// --- Mirror mappings ---

TEST(unicode_mirror_parentheses) {
    ASSERT_EQ(toMirror('('), (UNICODE_CODEPOINT)')');
    ASSERT_EQ(toMirror(')'), (UNICODE_CODEPOINT)'(');
}

TEST(unicode_mirror_brackets) {
    ASSERT_EQ(toMirror('['), (UNICODE_CODEPOINT)']');
    ASSERT_EQ(toMirror(']'), (UNICODE_CODEPOINT)'[');
}

TEST(unicode_mirror_braces) {
    ASSERT_EQ(toMirror('{'), (UNICODE_CODEPOINT)'}');
    ASSERT_EQ(toMirror('}'), (UNICODE_CODEPOINT)'{');
}

TEST(unicode_mirror_angle_brackets) {
    ASSERT_EQ(toMirror('<'), (UNICODE_CODEPOINT)'>');
    ASSERT_EQ(toMirror('>'), (UNICODE_CODEPOINT)'<');
}

TEST(unicode_mirror_no_mirror) {
    // Characters without mirrors return themselves
    ASSERT_EQ(toMirror('A'), (UNICODE_CODEPOINT)'A');
    ASSERT_EQ(toMirror(' '), (UNICODE_CODEPOINT)' ');
}

// --- Unicode traits ---

TEST(unicode_traits_space) {
    unicode_info_t traits = getTraitsForCodepoint(' ');
    ASSERT_TRUE(traits.is.linebreak);
}

TEST(unicode_traits_letter) {
    unicode_info_t traits = getTraitsForCodepoint('A');
    ASSERT_FALSE(traits.is.linebreak);
    ASSERT_FALSE(traits.is.nsm);
    ASSERT_FALSE(traits.is.controlchar);
}

TEST(unicode_traits_arabic_nsm) {
    // Fathah (U+064E) is a non-spacing mark
    unicode_info_t traits = getTraitsForCodepoint(0x064E);
    ASSERT_TRUE(traits.is.nsm);
}

TEST(unicode_traits_digit) {
    unicode_info_t traits = getTraitsForCodepoint('0');
    ASSERT_FALSE(traits.is.linebreak);
    ASSERT_FALSE(traits.is.nsm);
}
