/*
 * Tests for TextLayout::measureLineWrap and TextLayout::bytesForCodepoint.
 *
 * Uses MockGlyphProvider with width=8, so each character advances 8px.
 * At layoutWidth=80, exactly 10 characters fit on one line.
 */

#include "test_harness.hpp"
#include "MockGlyphProvider.hpp"
#include "TextLayout.hpp"
#include "Utf8.hpp"
#include <cstdlib>
#include <cstring>

using namespace focus;

// Helper: parse UTF-8 string into codepoint array. Caller must free().
static UNICODE_CODEPOINT* toCodepoints(const char* utf8, size_t* outLen) {
    size_t len = utf8_codepoint_length(utf8);
    UNICODE_CODEPOINT* cps = (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    utf8_parse(utf8, cps);
    *outLen = len;
    return cps;
}

// --- bytesForCodepoint ---

TEST(bytesForCodepoint_ascii) {
    ASSERT_EQ(TextLayout::bytesForCodepoint('A'), (size_t)1);
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x7F), (size_t)1);
}

TEST(bytesForCodepoint_two_byte) {
    // Latin Extended: é = U+00E9
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x00E9), (size_t)2);
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x07FF), (size_t)2);
}

TEST(bytesForCodepoint_two_byte_arabic) {
    // Arabic: ب = U+0628 (1576 decimal, fits in 2-byte range <= 0x7FF)
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x0628), (size_t)2);
}

TEST(bytesForCodepoint_three_byte) {
    // CJK: 中 = U+4E2D, BMP max = U+FFFF
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x4E2D), (size_t)3);
    ASSERT_EQ(TextLayout::bytesForCodepoint(0xFFFF), (size_t)3);
}

TEST(bytesForCodepoint_boundaries) {
    // Note: UNICODE_CODEPOINT is uint32_t (UNICODE_BMP_ONLY is not set in this build)
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x80), (size_t)2);   // first 2-byte
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x7FF), (size_t)2);  // last 2-byte
    ASSERT_EQ(TextLayout::bytesForCodepoint(0x800), (size_t)3);  // first 3-byte
}

// --- measureLineWrap ---

TEST(measureLineWrap_empty_input) {
    MockGlyphProvider gp(8);
    WordWrapResult r = TextLayout::measureLineWrap(nullptr, 0, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_FALSE(r.wrapped);
    ASSERT_FALSE(r.isParagraphBreak);
}

TEST(measureLineWrap_text_fits_on_one_line) {
    // "Hello" = 5 chars × 8px = 40px, fits in 80px
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hello", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);  // negative = no wrap needed
    ASSERT_FALSE(r.wrapped);
    ASSERT_EQ(r.endCursorX, (int16_t)40);

    free(cps);
}

TEST(measureLineWrap_exact_width_boundary) {
    // Regression test for eb58ce1: text width == layout width.
    // "ABCDEFGHIJ" = 10 chars × 8px = 80px, layoutWidth = 80.
    // With the <= fix, this should NOT wrap (cursorX reaches 80, but
    // the while loop condition is cursorX <= layoutWidth, so it checks
    // position >= len first and returns without wrapping).
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ABCDEFGHIJ", &len);
    ASSERT_EQ(len, (size_t)10);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    // All 10 chars fit (80px == 80px layout width), no wrap
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_FALSE(r.wrapped);
    ASSERT_EQ(r.endCursorX, (int16_t)80);

    free(cps);
}

TEST(measureLineWrap_overflow_wraps_at_word_boundary) {
    // "Hello World!" = 12 chars. "Hello " = 6×8 = 48px, "World!" = 6×8 = 48px.
    // Total = 96px > 80px. Should wrap after the space (position 5+1=6).
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hello World!", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)6);  // "Hello " consumed
    ASSERT_FALSE(r.isParagraphBreak);

    free(cps);
}

TEST(measureLineWrap_newline_paragraph_break) {
    // "Hi\nWorld" — newline at position 2, should consume 3 codepoints (including \n)
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hi\nWorld", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);  // "Hi\n"
    ASSERT_FALSE(r.wrapped);
    ASSERT_TRUE(r.isParagraphBreak);
    ASSERT_EQ(r.endCursorX, (int16_t)0);

    free(cps);
}

TEST(measureLineWrap_backspace_moves_cursor_back) {
    // "AB\bC" — A(8) B(8) BS(-8) C(8) = net 16px
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'A', 'B', 0x08, 'C'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 4, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)16);  // A(8) + B(8) - BS(8) + C(8) = 16

    // Actually: A advances to 8, B advances to 16, BS goes back to 8, C advances to 16
    // Yes, 16px total.
}

TEST(measureLineWrap_backspace_clamps_to_initial) {
    // Backspace at start shouldn't go negative
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {0x08, 'A'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 2, 80, 1, &gp);
    ASSERT_EQ(r.endCursorX, (int16_t)8);
}

TEST(measureLineWrap_control_chars_skipped) {
    // SOH (0x01) and other control chars < 0x20 (except \n, \b) are skipped
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {0x01, 'A', 0x02, 'B'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 4, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)16);  // only A and B have width
}

TEST(measureLineWrap_initial_cursor_x) {
    // Start at cursorX=40, 5 chars × 8 = 40px more → total 80px (fits exactly)
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hello", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp, 40);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)80);

    free(cps);
}

TEST(measureLineWrap_initial_cursor_x_overflow) {
    // Start at cursorX=40, 6 chars × 8 = 48px → total 88px, exceeds 80
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("He llo", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp, 40);
    ASSERT_TRUE(r.wrapped);
    ASSERT_GT(r.codepointsConsumed, (int32_t)0);

    free(cps);
}

TEST(measureLineWrap_single_word_wider_than_layout) {
    // "ABCDEFGHIJK" = 11 chars × 8px = 88px, no spaces, layout = 80px
    // Should force-break at position where we exceed width
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ABCDEFGHIJK", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    // No wrap candidate (no spaces), so forced break at the overflow position
    ASSERT_GT(r.codepointsConsumed, (int32_t)0);

    free(cps);
}

TEST(measureLineWrap_text_size_multiplier) {
    // textSize=2 means each glyph is 16px wide. "Hello" = 5 × 16 = 80px
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hello", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 2, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)80);

    free(cps);
}

TEST(measureLineWrap_text_size_multiplier_overflow) {
    // textSize=2, "Hello!" = 6 × 16 = 96px > 80
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("Hi Ho!", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 2, &gp);
    ASSERT_TRUE(r.wrapped);

    free(cps);
}

TEST(measureLineWrap_underline_same_width_as_plain) {
    // Underline pattern: _BS<char> for each character.
    // "_\x08H_\x08i" should measure the same width as "Hi" (16px).
    // Each _BS<char> triplet: _(8) + BS(-8) + char(8) = net 8px per character.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'_', 0x08, 'H', '_', 0x08, 'i'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 6, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)16);  // same as "Hi"
}

TEST(measureLineWrap_underline_wraps_like_plain_text) {
    // 10 underlined characters should fill exactly 80px, same as 10 plain chars.
    // Each _BS<char> = 3 codepoints, so 10 chars = 30 codepoints.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[30];
    for (int i = 0; i < 10; i++) {
        cps[i * 3]     = '_';
        cps[i * 3 + 1] = 0x08;
        cps[i * 3 + 2] = 'A' + i;
    }

    WordWrapResult r = TextLayout::measureLineWrap(cps, 30, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);  // fits exactly
    ASSERT_EQ(r.endCursorX, (int16_t)80);
}

TEST(measureLineWrap_underline_overflow_wraps) {
    // 11 underlined characters = 88px > 80px, should wrap.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[33];
    for (int i = 0; i < 11; i++) {
        cps[i * 3]     = '_';
        cps[i * 3 + 1] = 0x08;
        cps[i * 3 + 2] = 'A' + i;
    }

    WordWrapResult r = TextLayout::measureLineWrap(cps, 33, 80, 1, &gp);
    ASSERT_TRUE(r.wrapped);
}

TEST(measureLineWrap_so_si_control_chars_no_width) {
    // SO (0x0E) and SI (0x0F) are control characters (< 0x20), skipped by
    // measureLineWrap. They don't contribute width.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'A', 0x0E, 'B', 0x0F, 'C'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 5, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)24);  // only A, B, C have width
}

// --- soft hyphens (U+00AD) ---

// Hyphenator that records whether it was consulted; offers no positions.
struct RecordingHyphenator : public Hyphenator {
    mutable int calls = 0;
    size_t findBreakPositions(const UNICODE_CODEPOINT*, size_t,
                              size_t*, size_t) const override {
        calls++;
        return 0;
    }
};

TEST(measureLineWrap_soft_hyphen_zero_width) {
    // A mid-word soft hyphen contributes no width when no break is taken.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'b', 'c', 0x00AD, 'd', 'e', 'f'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 7, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)48);  // same as "abcdef"
}

TEST(measureLineWrap_soft_hyphen_break_sets_hyphen) {
    // "aaaa·bbbbbb" (·=SHY) in a 6-char line: the word overflows, the soft
    // hyphen is the break point, and the U+00AD is consumed onto the line.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'a', 'a', 'a', 0x00AD,
                               'b', 'b', 'b', 'b', 'b', 'b'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 11, 48, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_TRUE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)5);  // aaaa + the SHY
}

TEST(measureLineWrap_soft_hyphen_requires_hyphen_fit) {
    // The prefix fits, but prefix + hyphen would exceed the width, so the
    // soft hyphen is not a valid candidate — and it still suppresses the
    // hyphenator, so the line force-breaks with no hyphen.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'a', 'a', 'a', 'a', 'a', 0x00AD, 'b', 'b'};
    RecordingHyphenator hyph;

    // 6 chars = 48px fills the line exactly; 48 + 8 (hyphen) > 48.
    WordWrapResult r = TextLayout::measureLineWrap(cps, 9, 48, 1, &gp, 0,
                                                   FontStyle::Regular, &hyph);
    ASSERT_TRUE(r.wrapped);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(hyph.calls, 0);
}

TEST(measureLineWrap_soft_hyphen_suppresses_hyphenator) {
    // A word carrying soft hyphens never consults the hyphenator; the
    // author's break point wins.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'a', 'a', 'a', 0x00AD,
                               'b', 'b', 'b', 'b', 'b', 'b'};
    RecordingHyphenator hyph;

    WordWrapResult r = TextLayout::measureLineWrap(cps, 11, 48, 1, &gp, 0,
                                                   FontStyle::Regular, &hyph);
    ASSERT_TRUE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)5);
    ASSERT_EQ(hyph.calls, 0);
}

TEST(measureLineWrap_no_soft_hyphen_still_consults_hyphenator) {
    // Positive control: an overflowing word without soft hyphens still goes
    // to the hyphenator.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};
    RecordingHyphenator hyph;

    WordWrapResult r = TextLayout::measureLineWrap(cps, 8, 48, 1, &gp, 0,
                                                   FontStyle::Regular, &hyph);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(hyph.calls, 1);
}

TEST(measureLineWrap_multiple_soft_hyphens_rightmost_wins) {
    // "aaa·bbb·ccc" in a 7-char line: both soft hyphens fit with a hyphen;
    // the rightmost one is chosen (greedy, matching the hyphenator path).
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'a', 'a', 0x00AD, 'b', 'b', 'b', 0x00AD,
                               'c', 'c', 'c'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 11, 56, 1, &gp);
    ASSERT_TRUE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)8);  // aaa·bbb + the second SHY
}

TEST(measureLineWrap_soft_hyphen_before_space_not_used) {
    // A soft hyphen in an EARLIER word doesn't apply to the overflowing word:
    // the line breaks at the space, no hyphen.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'b', 0x00AD, 'c', 'd', ' ',
                               'e', 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 15, 64, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)6);  // through the space
}

TEST(measureTextWidth_ignores_soft_hyphen) {
    MockGlyphProvider gp(8);
    ASSERT_EQ(TextLayout::measureTextWidth("ab\xC2\xAD" "cd", 1, &gp),
              TextLayout::measureTextWidth("abcd", 1, &gp));
}

// --- trailing whitespace at the break ---
// The whitespace that ends a wrapped line paints nothing, so its advance is
// never charged against the layout width. Visible break opportunities
// (hyphens, ideographs) still have to fit.

// Hyphenator that records the word it was offered; offers no positions.
struct CapturingHyphenator : public Hyphenator {
    mutable int calls = 0;
    mutable UNICODE_CODEPOINT word[32];
    mutable size_t wordLen = 0;
    size_t findBreakPositions(const UNICODE_CODEPOINT* w, size_t n,
                              size_t*, size_t) const override {
        calls++;
        wordLen = n < 32 ? n : 32;
        for (size_t i = 0; i < wordLen; i++) word[i] = w[i];
        return 0;
    }
};

TEST(measureLineWrap_trailing_space_overflow_keeps_word) {
    // "ab cde" = 48px fills the line exactly; the space after "cde" would
    // land at 56. The word stays on the line and the space breaks it.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ab cde fg", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 48, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)7);  // "ab cde " consumed

    free(cps);
}

TEST(measureLineWrap_trailing_space_overflow_first_word) {
    // Same rule when there is no earlier break on the line.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("abcde fg", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 40, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)6);  // "abcde " consumed

    free(cps);
}

TEST(measureLineWrap_trailing_thin_space_overflow_keeps_word) {
    // U+2009 is whitespace with a break opportunity: same treatment as U+0020.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'b', 0x2009, 'c', 'd', 'e', 0x2009, 'f', 'g'};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 9, 48, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)7);
}

TEST(measureLineWrap_overflowing_ideograph_not_a_candidate) {
    // Every ideograph is a break opportunity, but a visible one that
    // overflows must not be pulled onto the line (8521a0d).
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {0x4E2D, 0x4E2D, 0x4E2D, 0x4E2D, 0x4E2D, 0x4E2D};

    WordWrapResult r = TextLayout::measureLineWrap(cps, 6, 40, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)5);
}

TEST(measureLineWrap_overflowing_hyphen_minus_not_a_candidate) {
    // A hyphen-minus that overflows is visible: the line breaks at the
    // earlier space instead.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ab cde-fg", &len);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 48, 1, &gp);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);  // "ab " consumed

    free(cps);
}

TEST(measureLineWrap_hyphenator_not_consulted_when_space_overflows) {
    // "ab cdefg" = 64px fits; the trailing space breaks the line, so there
    // is no overflowing word to hyphenate.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ab cdefg hi", &len);
    RecordingHyphenator hyph;

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 64, 1, &gp, 0,
                                                   FontStyle::Regular, &hyph);
    ASSERT_TRUE(r.wrapped);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)9);  // "ab cdefg " consumed
    ASSERT_EQ(hyph.calls, 0);

    free(cps);
}

TEST(measureLineWrap_hyphenator_receives_whole_word) {
    // The cursor crosses the edge at 'f', but the hyphenator is offered
    // "cdefghij" in full: Liang patterns are anchored on the whole word.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ab cdefghij", &len);
    CapturingHyphenator hyph;

    TextLayout::measureLineWrap(cps, len, 48, 1, &gp, 0,
                                FontStyle::Regular, &hyph);
    ASSERT_EQ(hyph.calls, 1);
    ASSERT_EQ(hyph.wordLen, (size_t)8);
    ASSERT_EQ(hyph.word[0], (UNICODE_CODEPOINT)'c');
    ASSERT_EQ(hyph.word[7], (UNICODE_CODEPOINT)'j');

    free(cps);
}

TEST(measureLineWrap_hyphenator_word_ends_at_next_space) {
    // The whole word, and no more: the word stops at the following space.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ab cdefghij kl", &len);
    CapturingHyphenator hyph;

    TextLayout::measureLineWrap(cps, len, 48, 1, &gp, 0,
                                FontStyle::Regular, &hyph);
    ASSERT_EQ(hyph.calls, 1);
    ASSERT_EQ(hyph.wordLen, (size_t)8);

    free(cps);
}

TEST(measureLineWrap_soft_hyphen_past_overflow_suppresses_hyphenator) {
    // "ab cdef·ghij": the cursor crosses the edge at 'f', before the soft
    // hyphen is reached. The author still owns this word's break points,
    // so the hyphenator is not consulted and the line breaks at the space.
    MockGlyphProvider gp(8);
    UNICODE_CODEPOINT cps[] = {'a', 'b', ' ', 'c', 'd', 'e', 'f', 0x00AD,
                               'g', 'h', 'i', 'j'};
    RecordingHyphenator hyph;

    WordWrapResult r = TextLayout::measureLineWrap(cps, 12, 48, 1, &gp, 0,
                                                   FontStyle::Regular, &hyph);
    ASSERT_TRUE(r.wrapped);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);
    ASSERT_EQ(hyph.calls, 0);
}
