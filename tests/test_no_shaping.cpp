/*
 * Flag-off proof: the whole framework compiles with
 * FOCUS_HAS_TEXT_SHAPING=0, shapeArabic is inert, and measurement
 * stays consistent with rendering (both unshaped).
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
#include <cstring>
#include <cstdio>

using namespace focus;

TEST(no_shaping_shapeArabic_is_inert) {
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0627};
    shapeArabicIfNeeded(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0x0644);
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x0627);
}

TEST(no_shaping_measurement_is_unshaped_and_consistent) {
    // With shaping off, lam+alef measures as two 8px glyphs — matching
    // what the renderer will draw (also unshaped). Consistency, not
    // correctness, is the flag-off promise.
    MockGlyphProvider provider(8, 12);
    ASSERT_EQ(TextLayout::measureTextWidth("\xD9\x84\xD8\xA7", 1, &provider), (int16_t)16);
}

int main() {
    return runAllTests();
}
