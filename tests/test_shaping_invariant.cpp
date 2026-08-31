/*
 * Pins for the shaping invariant: shapeArabicIfNeeded detects and shapes
 * in place, and shaping is idempotent — already-shaped text passes
 * through byte-identical. libros's reader depends on the pass-through.
 */

#include "test_harness.hpp"
#include "ArabicShaping.hpp"
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
