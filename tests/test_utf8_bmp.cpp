/*
 * Tests for the UTF-8 decoder compiled with UNICODE_BMP_ONLY.
 *
 * This file builds as its own executable (utf8-bmp-test) with UNICODE_BMP_ONLY
 * defined, linking its own copy of Utf8.cpp — the only place in the tree that
 * exercises the 16-bit codepoint configuration. Under the flag,
 * UNICODE_CODEPOINT is uint16_t, the sentinels are the BMP noncharacters
 * (UTF8_END = 0xFFFF, UTF8_ERROR = 0xFFFE), and valid codepoints that don't
 * fit 16 bits decode as UTF8_REPLACEMENT_CHARACTER.
 */

#include "test_harness.hpp"
#include "Utf8.hpp"
#include <cstring>

using namespace focus;

TEST(bmp_codepoint_is_16_bit) {
    ASSERT_EQ(sizeof(UNICODE_CODEPOINT), (size_t)2);
    ASSERT_EQ((uint32_t)UTF8_END, (uint32_t)0xFFFF);
    ASSERT_EQ((uint32_t)UTF8_ERROR, (uint32_t)0xFFFE);
}

TEST(bmp_decodes_bmp_codepoints) {
    UNICODE_CODEPOINT cps[4];
    ASSERT_EQ(utf8_parse("a\xC3\xA9\xE2\x82\xAC", cps), (size_t)3);  // a é €
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)'a');
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0xE9);
    ASSERT_EQ(cps[2], (UNICODE_CODEPOINT)0x20AC);
}

TEST(bmp_astral_codepoint_becomes_replacement) {
    // 😀 = U+1F600 doesn't fit in 16 bits: decodes as U+FFFD, not an error.
    UNICODE_CODEPOINT cps[4];
    ASSERT_EQ(utf8_parse("\xF0\x9F\x98\x80", cps), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)UTF8_REPLACEMENT_CHARACTER);
}

TEST(bmp_noncharacters_become_replacement) {
    // U+FFFE and U+FFFF are valid encodings but collide with the 16-bit
    // sentinels, so they decode as U+FFFD too.
    UNICODE_CODEPOINT cps[4];
    ASSERT_EQ(utf8_parse("\xEF\xBF\xBE", cps), (size_t)1);  // U+FFFE
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)UTF8_REPLACEMENT_CHARACTER);
    ASSERT_EQ(utf8_parse("\xEF\xBF\xBF", cps), (size_t)1);  // U+FFFF
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)UTF8_REPLACEMENT_CHARACTER);
}

TEST(bmp_invalid_input_still_rejected) {
    ASSERT_EQ(utf8_codepoint_length("\xC0\xAF"), (size_t)0);      // overlong
    ASSERT_EQ(utf8_codepoint_length("\xED\xA0\x80"), (size_t)0);  // surrogate
    const char* text = "\x80";
    const char* cursor = text;
    ASSERT_EQ(utf8_next(cursor, text + 1), (UNICODE_CODEPOINT)UTF8_ERROR);
}

int main() {
    return runAllTests();
}
