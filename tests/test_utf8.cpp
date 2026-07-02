/*
 * Tests for the Focus UTF-8 decoder (Utf8.hpp): utf8_next, utf8_parse,
 * utf8_codepoint_length, and the streaming utf8DecodeStep core.
 *
 * Note on history: the Crockford decoder this replaced had broken error
 * detection for malformed 2- and 3-byte sequences (positive sentinels defeated
 * its `>= 0` continuation-byte guards), so several of the invalid-input cases
 * below pin behavior the old decoder got wrong.
 */

#include "test_harness.hpp"
#include "Utf8.hpp"
#include <cstring>

using namespace focus;

// Helper: run utf8_parse into a fixed buffer, returning the count.
static size_t parseInto(const char* utf8, UNICODE_CODEPOINT* buf, size_t bufLen) {
    memset(buf, 0, bufLen * sizeof(UNICODE_CODEPOINT));
    return utf8_parse(utf8, buf);
}

// --- valid input: utf8_parse / utf8_codepoint_length ---

TEST(utf8_ascii) {
    UNICODE_CODEPOINT cps[8];
    ASSERT_EQ(parseInto("Az", cps, 8), (size_t)2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)'A');
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)'z');
    ASSERT_EQ(utf8_codepoint_length("Az"), (size_t)2);
}

TEST(utf8_two_byte) {
    UNICODE_CODEPOINT cps[8];
    // é = U+00E9 (C3 A9)
    ASSERT_EQ(parseInto("\xC3\xA9", cps, 8), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x00E9);
}

TEST(utf8_three_byte) {
    UNICODE_CODEPOINT cps[8];
    // € = U+20AC (E2 82 AC)
    ASSERT_EQ(parseInto("\xE2\x82\xAC", cps, 8), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x20AC);
}

TEST(utf8_four_byte) {
    UNICODE_CODEPOINT cps[8];
    // 😀 = U+1F600 (F0 9F 98 80)
    ASSERT_EQ(parseInto("\xF0\x9F\x98\x80", cps, 8), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x1F600);
}

TEST(utf8_boundary_codepoints) {
    UNICODE_CODEPOINT cps[8];
    ASSERT_EQ(parseInto("\x7F", cps, 8), (size_t)1);              // U+007F: last 1-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x7F);
    ASSERT_EQ(parseInto("\xC2\x80", cps, 8), (size_t)1);          // U+0080: first 2-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x80);
    ASSERT_EQ(parseInto("\xDF\xBF", cps, 8), (size_t)1);          // U+07FF: last 2-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x7FF);
    ASSERT_EQ(parseInto("\xE0\xA0\x80", cps, 8), (size_t)1);      // U+0800: first 3-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x800);
    ASSERT_EQ(parseInto("\xEF\xBF\xBF", cps, 8), (size_t)1);      // U+FFFF: last 3-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFFFF);
    ASSERT_EQ(parseInto("\xF0\x90\x80\x80", cps, 8), (size_t)1);  // U+10000: first 4-byte
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x10000);
    ASSERT_EQ(parseInto("\xF4\x8F\xBF\xBF", cps, 8), (size_t)1);  // U+10FFFF: last valid
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x10FFFF);
}

TEST(utf8_last_before_surrogates_valid) {
    UNICODE_CODEPOINT cps[8];
    // U+D7FF (ED 9F BF) is valid; the surrogate block starts at U+D800.
    ASSERT_EQ(parseInto("\xED\x9F\xBF", cps, 8), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xD7FF);
    // U+E000 (EE 80 80) is valid; first codepoint after the surrogate block.
    ASSERT_EQ(parseInto("\xEE\x80\x80", cps, 8), (size_t)1);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xE000);
}

TEST(utf8_mixed_string) {
    UNICODE_CODEPOINT cps[8];
    // "aé€😀" — 1-, 2-, 3-, and 4-byte sequences in one string.
    ASSERT_EQ(parseInto("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80", cps, 8), (size_t)4);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)'a');
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0xE9);
    ASSERT_EQ(cps[2], (UNICODE_CODEPOINT)0x20AC);
    ASSERT_EQ(cps[3], (UNICODE_CODEPOINT)0x1F600);
}

TEST(utf8_empty_string) {
    UNICODE_CODEPOINT cps[2];
    ASSERT_EQ(parseInto("", cps, 2), (size_t)0);
    ASSERT_EQ(utf8_codepoint_length(""), (size_t)0);
}

TEST(utf8_parse_null_buf_counts) {
    ASSERT_EQ(utf8_parse("a\xC3\xA9", NULL), (size_t)2);
}

// --- invalid input: overlong encodings ---

TEST(utf8_rejects_overlong_two_byte) {
    // C0 AF and C1 81 encode ASCII values in two bytes.
    ASSERT_EQ(utf8_codepoint_length("\xC0\xAF"), (size_t)0);
    ASSERT_EQ(utf8_codepoint_length("\xC1\x81"), (size_t)0);
}

TEST(utf8_rejects_overlong_three_byte) {
    // E0 80 AF encodes U+002F in three bytes.
    ASSERT_EQ(utf8_codepoint_length("\xE0\x80\xAF"), (size_t)0);
    // E0 9F BF encodes U+07FF in three bytes (last overlong 3-byte).
    ASSERT_EQ(utf8_codepoint_length("\xE0\x9F\xBF"), (size_t)0);
}

TEST(utf8_rejects_overlong_four_byte) {
    // F0 80 80 AF encodes U+002F in four bytes.
    ASSERT_EQ(utf8_codepoint_length("\xF0\x80\x80\xAF"), (size_t)0);
    // F0 8F BF BF encodes U+FFFF in four bytes (last overlong 4-byte).
    ASSERT_EQ(utf8_codepoint_length("\xF0\x8F\xBF\xBF"), (size_t)0);
}

// --- invalid input: surrogates and out-of-range ---

TEST(utf8_rejects_surrogates) {
    ASSERT_EQ(utf8_codepoint_length("\xED\xA0\x80"), (size_t)0);  // U+D800
    ASSERT_EQ(utf8_codepoint_length("\xED\xBF\xBF"), (size_t)0);  // U+DFFF
}

TEST(utf8_rejects_beyond_unicode_range) {
    ASSERT_EQ(utf8_codepoint_length("\xF4\x90\x80\x80"), (size_t)0);  // U+110000
    ASSERT_EQ(utf8_codepoint_length("\xF5\x80\x80\x80"), (size_t)0);  // lead F5
    ASSERT_EQ(utf8_codepoint_length("\xF8\x80"), (size_t)0);          // lead F8
    ASSERT_EQ(utf8_codepoint_length("\xFF"), (size_t)0);              // lead FF
}

// --- invalid input: stray / truncated / wrong continuation bytes ---

TEST(utf8_rejects_stray_continuation) {
    ASSERT_EQ(utf8_codepoint_length("\x80"), (size_t)0);
    ASSERT_EQ(utf8_codepoint_length("A\x80"), (size_t)0);
}

TEST(utf8_rejects_truncated_sequences) {
    ASSERT_EQ(utf8_codepoint_length("\xC3"), (size_t)0);          // 2-byte, no tail
    ASSERT_EQ(utf8_codepoint_length("\xE2\x82"), (size_t)0);      // 3-byte, one short
    ASSERT_EQ(utf8_codepoint_length("\xF0\x9F\x98"), (size_t)0);  // 4-byte, one short
}

TEST(utf8_rejects_bad_continuation_bytes) {
    // These are the cases the old Crockford adaptation decoded to garbage:
    // a lead byte followed by a non-continuation byte.
    ASSERT_EQ(utf8_codepoint_length("\xC3\x41"), (size_t)0);      // 2-byte, ASCII tail
    ASSERT_EQ(utf8_codepoint_length("\xE2\x41\x82"), (size_t)0);  // 3-byte, ASCII tail
    ASSERT_EQ(utf8_codepoint_length("\xF0\x41\x80\x80"), (size_t)0);
}

// --- utf8_next: cursor semantics ---

TEST(utf8_next_iterates_and_ends) {
    const char* text = "a\xC3\xA9";
    const char* cursor = text;
    const char* end = text + strlen(text);
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)'a');
    ASSERT_EQ(cursor, text + 1);
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)0xE9);
    ASSERT_EQ(cursor, end);
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)UTF8_END);
    ASSERT_EQ(cursor, end);  // END does not move the cursor
}

TEST(utf8_next_error_leaves_cursor) {
    const char* text = "a\x80z";
    const char* cursor = text;
    const char* end = text + strlen(text);
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)'a');
    const char* beforeError = cursor;
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)UTF8_ERROR);
    ASSERT_EQ(cursor, beforeError);  // error does not advance
}

TEST(utf8_next_truncated_at_end_is_error) {
    const char* text = "\xE2\x82";  // 3-byte sequence cut short by the buffer
    const char* cursor = text;
    const char* end = text + 2;
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)UTF8_ERROR);
    ASSERT_EQ(cursor, text);
}

TEST(utf8_next_respects_end_not_nul) {
    // A NUL byte inside the range is just U+0000; the end pointer is the limit.
    const char text[] = { 'a', 0, 'b' };
    const char* cursor = text;
    const char* end = text + 3;
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)'a');
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)0);
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)'b');
    ASSERT_EQ(utf8_next(cursor, end), (UNICODE_CODEPOINT)UTF8_END);
}

// --- utf8DecodeStep: streaming core ---

TEST(utf8_decode_step_streams_bytes) {
    // Decode € (U+20AC) one byte at a time.
    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0;
    ASSERT_NE(utf8DecodeStep(state, codepoint, 0xE2), (uint32_t)UTF8_ACCEPT);
    ASSERT_NE(state, (uint32_t)UTF8_REJECT);
    ASSERT_NE(utf8DecodeStep(state, codepoint, 0x82), (uint32_t)UTF8_ACCEPT);
    ASSERT_NE(state, (uint32_t)UTF8_REJECT);
    ASSERT_EQ(utf8DecodeStep(state, codepoint, 0xAC), (uint32_t)UTF8_ACCEPT);
    ASSERT_EQ(codepoint, (uint32_t)0x20AC);
}

TEST(utf8_decode_step_rejects_bad_byte) {
    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0;
    utf8DecodeStep(state, codepoint, 0xE2);
    ASSERT_EQ(utf8DecodeStep(state, codepoint, 0x41), (uint32_t)UTF8_REJECT);
}
