/*
 * Paragraph direction: each paragraph's first strong character decides
 * whether it lays out left to right or right to left (UAX #9 P2/P3).
 */

#include "test_harness.hpp"
#include "TextLayout.hpp"

using namespace focus;

static const UNICODE_CODEPOINT SHIN = 0x05E9, LAMED = 0x05DC, VAV = 0x05D5, FINAL_MEM = 0x05DD;

TEST(paragraph_direction_hebrew_is_rtl) {
    UNICODE_CODEPOINT cps[] = {SHIN, LAMED, VAV, FINAL_MEM};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 4), -1);
}

TEST(paragraph_direction_latin_first_is_ltr) {
    UNICODE_CODEPOINT cps[] = {'W', 'i', 'F', 'i', ' ', SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 6), 1);
}

TEST(paragraph_direction_digits_do_not_decide) {
    UNICODE_CODEPOINT cps[] = {'1', '2', '3', ' ', SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 5), -1);
}

TEST(paragraph_direction_leading_neutrals_are_skipped) {
    UNICODE_CODEPOINT cps[] = {'.', ' ', SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 3), -1);
}

TEST(paragraph_direction_empty_is_ltr) {
    UNICODE_CODEPOINT cps[] = {SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 0), 1);
}

TEST(paragraph_direction_no_strong_character_is_ltr) {
    UNICODE_CODEPOINT cps[] = {'1', '2', '3'};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 3), 1);
}

TEST(paragraph_direction_rlm_decides) {
    UNICODE_CODEPOINT cps[] = {0x200F, 'a', 'b', 'c'};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 4), -1);
}

TEST(paragraph_direction_lrm_decides) {
    UNICODE_CODEPOINT cps[] = {0x200E, SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 2), 1);
}

TEST(paragraph_direction_stops_at_newline) {
    UNICODE_CODEPOINT cps[] = {'1', '\n', SHIN};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 3), 1);
    UNICODE_CODEPOINT two[] = {SHIN, '\n', 'H', 'i'};
    ASSERT_EQ(TextLayout::paragraphDirection(two, 4), -1);
    ASSERT_EQ(TextLayout::paragraphDirection(two + 2, 2), 1);
}

TEST(paragraph_direction_shaped_arabic_is_rtl) {
    // Beh initial + lam-alef final ligature + the U+200B the shaper leaves.
    UNICODE_CODEPOINT cps[] = {0xFE91, 0xFEFC, 0x200B};
    ASSERT_EQ(TextLayout::paragraphDirection(cps, 3), -1);
}
