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
#include <vector>

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
    // so this is the load-bearing pass-through contract.
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
