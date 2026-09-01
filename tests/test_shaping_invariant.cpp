/*
 * Pins for the shaping invariant: shapeArabicIfNeeded detects and shapes
 * in place, and shaping is idempotent — already-shaped text passes
 * through byte-identical. libros's reader depends on the pass-through.
 */

#include "test_harness.hpp"
#include "ArabicShaping.hpp"
#include "TextLayout.hpp"
#include "MockGlyphProvider.hpp"
#include <cstring>

using namespace focus;

TEST(shape_if_needed_shapes_lam_alef) {
    // Lam + alef with no joining context becomes the isolated ligature
    // plus a zero-width space.
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0627};
    shapeArabicIfNeeded(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFEFB);
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x200B);
}

TEST(shape_if_needed_leaves_latin_untouched) {
    UNICODE_CODEPOINT cps[] = {'H', 'i', ' ', '!', 0x2026};
    UNICODE_CODEPOINT before[5];
    memcpy(before, cps, sizeof(cps));
    shapeArabicIfNeeded(cps, 5);
    ASSERT_EQ(memcmp(cps, before, sizeof(cps)), 0);
}

TEST(shaping_is_idempotent) {
    // Shape a mixed buffer twice; the second pass must change nothing.
    // The detect range (U+0621-U+06D2) excludes shapeArabic's own output,
    // so this is the pass-through contract.
    UNICODE_CODEPOINT cps[] = {'a', ' ', 0x0644, 0x0627, ' ', 0x0645, 'z'};
    size_t len = sizeof(cps) / sizeof(cps[0]);
    shapeArabicIfNeeded(cps, len);
    UNICODE_CODEPOINT once[sizeof(cps) / sizeof(cps[0])];
    memcpy(once, cps, sizeof(cps));
    shapeArabicIfNeeded(cps, len);
    ASSERT_EQ(memcmp(cps, once, sizeof(cps)), 0);
    // And the direct shaper is equally idempotent on shaped input.
    shapeArabic(cps, len);
    ASSERT_EQ(memcmp(cps, once, sizeof(cps)), 0);
}

// Mock that gives U+200B its correct zero advance so ligature collapse
// is observable; everything else keeps the fixed 8px advance.
class ZeroWidthAwareProvider : public MockGlyphProvider {
public:
    ZeroWidthAwareProvider() : MockGlyphProvider(8, 12) {}
    focus::GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint,
                                            focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        if (codepoint == 0x200B) return focus::GlyphMetrics{0, 0, 12, 0, 0};
        return MockGlyphProvider::metricsForCodepoint(codepoint, emphasis);
    }
};

TEST(measure_text_width_counts_lam_alef_once) {
    // Unshaped lam+alef is two glyphs; shaped it is one ligature plus a
    // zero-width space. The invariant means measurement sees the ligature.
    ZeroWidthAwareProvider provider;
    int16_t w = TextLayout::measureTextWidth("\xD9\x84\xD8\xA7", 1, &provider);
    ASSERT_EQ(w, (int16_t)8);
}

TEST(measure_text_width_pre_shaped_equals_logical) {
    // Passing the already-shaped string measures the same as the logical
    // string: internal shaping must not double-transform shaped input.
    ZeroWidthAwareProvider provider;
    int16_t logical = TextLayout::measureTextWidth("\xD9\x84\xD8\xA7", 1, &provider);
    int16_t shaped = TextLayout::measureTextWidth("\xEF\xBB\xBB\xE2\x80\x8B", 1, &provider);
    ASSERT_EQ(logical, shaped);
}

TEST(measure_text_height_single_newline_charges_line_height) {
    // "a\nb" at generous width: line "a\n" charges lineHeight (14), the
    // final line charges glyph rows (12). Old model charged the newline
    // paragraphHeight (16) for a total of 28.
    MockGlyphProvider provider(8, 12);
    ASSERT_EQ(TextLayout::measureTextHeight("a\nb", 80, 1, &provider), (int16_t)26);
}

TEST(measure_text_height_blank_line_charges_paragraph_spacing) {
    // "a\n\nb": 14 (a's line) + 4 (blank line: paragraphSpacing alone,
    // mirroring writeCodepoint's consecutive-newline branch) + 12 (b).
    // Old model: 16 + 16 + 12 = 44.
    MockGlyphProvider provider(8, 12);
    ASSERT_EQ(TextLayout::measureTextHeight("a\n\nb", 80, 1, &provider), (int16_t)30);
}

TEST(measure_text_height_leading_blank_line_is_a_line_break) {
    // "\nb": the first \n has no preceding newline, so it charges
    // lineHeight (14), not paragraphSpacing — exactly as the renderer's
    // lastWasNewline starts false. Total 14 + 12 = 26.
    MockGlyphProvider provider(8, 12);
    ASSERT_EQ(TextLayout::measureTextHeight("\nb", 80, 1, &provider), (int16_t)26);
}

TEST(measure_text_height_wraps_on_shaped_widths) {
    // "لا لا لا" at width 24. Shaped, each word is one 8px ligature plus a
    // zero-width space, so two lines fit: 14 + 12 = 26. Unshaped the words
    // are 16px wide and it takes three lines: 14 + 14 + 12 = 40.
    ZeroWidthAwareProvider provider;
    const char* text = "\xD9\x84\xD8\xA7 \xD9\x84\xD8\xA7 \xD9\x84\xD8\xA7";
    ASSERT_EQ(TextLayout::measureTextHeight(text, 24, 1, &provider), (int16_t)26);
}

TEST(measure_text_height_shapes_arabic) {
    // Pre-shaped and logical strings wrap identically, so heights agree.
    ZeroWidthAwareProvider provider;
    int16_t logical = TextLayout::measureTextHeight("\xD9\x84\xD8\xA7", 40, 1, &provider);
    int16_t shaped = TextLayout::measureTextHeight("\xEF\xBB\xBB\xE2\x80\x8B", 40, 1, &provider);
    ASSERT_EQ(logical, shaped);
}
