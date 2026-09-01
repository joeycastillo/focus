/*
 * Flag-off proof: the whole framework compiles with
 * FOCUS_HAS_TEXT_SHAPING=0, shapeArabic is inert, and measurement sees
 * unshaped codepoints — the renderer shares the same inert call.
 *
 * Built only as the focus-test-noshaping executable. Like
 * test_no_filesystem.cpp it carries its own main; the harness's
 * registry and runAllTests() are header-inline, so main.cpp is not
 * linked in.
 */

#include "test_harness.hpp"
#include "MockGlyphProvider.hpp"
#include "ArabicShaping.hpp"
#include "TextLayout.hpp"

using namespace focus;

// Mock that gives U+200B its correct zero advance, so a shaped lam-alef
// (ligature plus zero-width space) would measure 8px where the unshaped
// pair measures 16px.
class ZeroWidthAwareProvider : public MockGlyphProvider {
public:
    ZeroWidthAwareProvider() : MockGlyphProvider(8, 12) {}
    focus::GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint,
                                            focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        if (codepoint == 0x200B) return focus::GlyphMetrics{0, 0, 12, 0, 0};
        return MockGlyphProvider::metricsForCodepoint(codepoint, emphasis);
    }
};

TEST(no_shaping_shapeArabic_is_inert) {
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0627};
    shapeArabicIfNeeded(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x0644);
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x0627);
}

TEST(no_shaping_measurement_is_unshaped_and_consistent) {
    // Lam+alef stays two 8px glyphs. Shaping would collapse it to the
    // 8px ligature plus a zero-width space, so 16 only holds when
    // shapeArabic really is compiled out.
    ZeroWidthAwareProvider provider;
    ASSERT_EQ(TextLayout::measureTextWidth("\xD9\x84\xD8\xA7", 1, &provider), (int16_t)16);
}

int main() {
    return runAllTests();
}
