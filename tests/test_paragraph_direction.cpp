/*
 * Paragraph direction: each paragraph's first strong character decides
 * whether it lays out left to right or right to left (UAX #9 P2/P3).
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "MockGlyphProvider.hpp"
#include "Window.hpp"
#include "LabelView.hpp"
#include "TextView.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

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

// --- Rendering ---
// Solid 8px glyphs, except '.' and U+2026, which are blank with full
// advance, so where they land shows as a gap in the ink.

class BlankPunctuationProvider : public MockGlyphProvider {
public:
    BlankPunctuationProvider() : MockGlyphProvider(8, 12) {
        memset(this->bitmap, 0xFF, sizeof(this->bitmap));
    }
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint,
                                     FontStyle emphasis = FontStyle::Regular) const override {
        if (codepoint == '.' || codepoint == 0x2026) return this->blank;
        return MockGlyphProvider::glyphForCodepoint(codepoint, emphasis);
    }
private:
    uint8_t blank[32 * 6] = {};
};

static std::shared_ptr<Font> makeBlankPunctuationFont() {
    return Font::withProvider(std::make_shared<BlankPunctuationProvider>());
}

// "שלום" (Hebrew, four strong R glyphs).
#define HEB "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"

static std::shared_ptr<RecordingDisplay> drawLabel(const char* text, int width, int height,
                                                   TextAlignment alignment,
                                                   TruncationMode truncation = TruncationMode::None) {
    auto display = std::make_shared<RecordingDisplay>(80, 40);
    auto window = std::make_shared<Window>(display, MakeSize(80, 40));
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, width, height), text);
    label->setFont(makeBlankPunctuationFont());
    label->setTextAlignment(alignment);
    label->setTruncationMode(truncation);
    window->addSubview(label);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 40));
    return display;
}

TEST(rtl_label_period_lands_at_left_end) {
    // Letters at 8..39, period gap at 0..7: text still starts at x=0.
    auto d = drawLabel(HEB ".", 80, 40, TextAlignment::Left);
    ASSERT_EQ(d->pixel(3, 5), 0xFF);
    ASSERT_EQ(d->pixel(8, 5), 1);
    ASSERT_EQ(d->pixel(39, 5), 1);
    ASSERT_EQ(d->pixel(40, 5), 0xFF);
}

TEST(latin_first_label_stays_ltr) {
    // "WiFi " at 0..39, Hebrew at 40..71, period gap at 72..79.
    auto d = drawLabel("WiFi " HEB ".", 80, 40, TextAlignment::Left);
    ASSERT_EQ(d->pixel(0, 5), 1);
    ASSERT_EQ(d->pixel(71, 5), 1);
    ASSERT_EQ(d->pixel(75, 5), 0xFF);
}

TEST(label_paragraphs_choose_direction_independently) {
    auto d = drawLabel(HEB ".\nHello.", 80, 40, TextAlignment::Left);
    ASSERT_EQ(d->pixel(3, 5), 0xFF);    // RTL: period at the left end
    ASSERT_EQ(d->pixel(8, 5), 1);
    ASSERT_EQ(d->pixel(0, 19), 1);      // LTR: period at the right end
    ASSERT_EQ(d->pixel(39, 19), 1);
    ASSERT_EQ(d->pixel(44, 19), 0xFF);
}

TEST(wrapped_rtl_line_starting_latin_stays_rtl) {
    // 48px wraps after the space; line 2 "abc." is still in an RTL paragraph.
    auto d = drawLabel(HEB " abc.", 48, 40, TextAlignment::Left);
    ASSERT_EQ(d->pixel(0, 5), 1);
    ASSERT_EQ(d->pixel(31, 5), 1);
    ASSERT_EQ(d->pixel(3, 19), 0xFF);   // period gap at the left end
    ASSERT_EQ(d->pixel(8, 19), 1);      // "abc" at 8..31
    ASSERT_EQ(d->pixel(31, 19), 1);
}

TEST(rtl_label_right_alignment) {
    // Letters flush right at 48..79, period gap at 40..47.
    auto d = drawLabel(HEB ".", 80, 40, TextAlignment::Right);
    ASSERT_EQ(d->pixel(43, 5), 0xFF);
    ASSERT_EQ(d->pixel(48, 5), 1);
    ASSERT_EQ(d->pixel(79, 5), 1);
}

TEST(rtl_label_center_alignment) {
    // 40px line centred in 80: period gap at 20..27, letters at 28..59.
    auto d = drawLabel(HEB ".", 80, 40, TextAlignment::Center);
    ASSERT_EQ(d->pixel(24, 5), 0xFF);
    ASSERT_EQ(d->pixel(28, 5), 1);
    ASSERT_EQ(d->pixel(59, 5), 1);
    ASSERT_EQ(d->pixel(60, 5), 0xFF);
}

TEST(rtl_tail_truncation_puts_ellipsis_at_left_end) {
    // One 12px line: "שלום…" fits 40px; the ellipsis gap lands at 0..7.
    auto d = drawLabel(HEB " " HEB, 40, 12, TextAlignment::Left, TruncationMode::Tail);
    ASSERT_EQ(d->pixel(3, 5), 0xFF);
    ASSERT_EQ(d->pixel(8, 5), 1);
    ASSERT_EQ(d->pixel(39, 5), 1);
}

// --- TextView ---

static std::shared_ptr<RecordingDisplay> drawTextView(const char* text, int width,
                                                      TextAlignment alignment) {
    auto display = std::make_shared<RecordingDisplay>(80, 40);
    auto window = std::make_shared<Window>(display, MakeSize(80, 40));
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, width, 40), text);
    tv->setFont(makeBlankPunctuationFont());
    tv->setTextAlignment(alignment);
    window->addSubview(tv);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 40));
    return display;
}

// LabelView and TextView must draw RTL paragraphs pixel-identically.
static void assertRtlParity(const char* text, int width, TextAlignment alignment) {
    auto label = drawLabel(text, width, 40, alignment);
    auto tv = drawTextView(text, width, alignment);
    ASSERT_TRUE(std::count(tv->framebuffer.begin(), tv->framebuffer.end(), (uint8_t)1) > 0);
    ASSERT_TRUE(label->framebuffer == tv->framebuffer);
}

TEST(text_view_rtl_parity_left) {
    assertRtlParity(HEB ".", 80, TextAlignment::Left);
}

TEST(text_view_rtl_parity_right) {
    assertRtlParity(HEB ".", 80, TextAlignment::Right);
}

TEST(text_view_rtl_parity_center) {
    assertRtlParity(HEB ".", 80, TextAlignment::Center);
}

TEST(text_view_rtl_parity_wrapped_latin_start) {
    assertRtlParity(HEB " abc.", 48, TextAlignment::Left);
}

TEST(text_view_paragraphs_choose_direction_independently) {
    auto d = drawTextView(HEB ".\nHello.", 80, TextAlignment::Left);
    ASSERT_EQ(d->pixel(3, 5), 0xFF);    // RTL: period at the left end
    ASSERT_EQ(d->pixel(8, 5), 1);
    ASSERT_EQ(d->pixel(0, 19), 1);      // LTR: period at the right end
    ASSERT_EQ(d->pixel(39, 19), 1);
    ASSERT_EQ(d->pixel(44, 19), 0xFF);
}
