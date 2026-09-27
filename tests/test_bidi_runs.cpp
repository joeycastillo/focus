/*
 * Directional runs within a line: justification extras travel with the run
 * whose spaces receive them, and a synthesized trailing hyphen follows the
 * line's last character in that character's direction.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "MockGlyphProvider.hpp"
#include "Window.hpp"
#include "LabelView.hpp"
#include "TextView.hpp"
#include "Font.hpp"
#include <algorithm>
#include <cstring>
#include <memory>

using namespace focus;

// Solid 8px glyphs, except ' ', which is blank, and '-', which inks only
// row 6, so a hyphen reads differently from a letter.
class HyphenStrokeProvider : public MockGlyphProvider {
public:
    HyphenStrokeProvider() : MockGlyphProvider(8, 12) {
        memset(this->bitmap, 0xFF, sizeof(this->bitmap));
        this->stroke[6] = 0xFF;
    }
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint,
                                     FontStyle emphasis = FontStyle::Regular) const override {
        if (codepoint == ' ') return this->blank;
        if (codepoint == '-') return this->stroke;
        return MockGlyphProvider::glyphForCodepoint(codepoint, emphasis);
    }
private:
    uint8_t stroke[32 * 6] = {};
    uint8_t blank[32 * 6] = {};
};

// "שש" (two strong R glyphs).
#define HE "\xD7\xA9\xD7\xA9"
#define SHY "\xC2\xAD"

static std::shared_ptr<RecordingDisplay> draw(bool textView, const char* text, int width,
                                              TextAlignment alignment) {
    auto display = std::make_shared<RecordingDisplay>(80, 40);
    auto window = std::make_shared<Window>(display, MakeSize(80, 40));
    auto font = Font::withProvider(std::make_shared<HyphenStrokeProvider>());
    if (textView) {
        auto tv = std::make_shared<TextView>(MakeRect(0, 0, width, 40), text);
        tv->setFont(font);
        tv->setTextAlignment(alignment);
        window->addSubview(tv);
    } else {
        auto label = std::make_shared<LabelView>(MakeRect(0, 0, width, 40), text);
        label->setFont(font);
        label->setTextAlignment(alignment);
        window->addSubview(label);
    }
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 40));
    return display;
}

// A word swapped for one of the other direction and the same width must
// leave the drawing unchanged, since every glyph is the same solid block.
static void assertTwins(bool textView, const char* mixed, const char* single, int width = 80) {
    auto a = draw(textView, mixed, width, TextAlignment::Justified);
    auto b = draw(textView, single, width, TextAlignment::Justified);
    ASSERT_TRUE(std::count(b->framebuffer.begin(), b->framebuffer.end(), (uint8_t)1) > 0);
    ASSERT_TRUE(a->framebuffer == b->framebuffer);
}

// --- Justified lines with more than one directional run ---
// Line 1 is three two-glyph words and a trailing space: two gaps share
// 16px of slack.

TEST(justified_rtl_paragraph_with_latin_word_label) {
    assertTwins(false, HE " " HE " ab " HE " " HE, HE " " HE " " HE " " HE " " HE);
}

TEST(justified_rtl_paragraph_with_latin_word_text_view) {
    assertTwins(true, HE " " HE " ab " HE " " HE, HE " " HE " " HE " " HE " " HE);
}

TEST(justified_ltr_paragraph_with_hebrew_words_label) {
    assertTwins(false, "ab " HE " " HE " ab ab", "ab ab ab ab ab");
}

TEST(justified_ltr_paragraph_with_hebrew_words_text_view) {
    assertTwins(true, "ab " HE " " HE " ab ab", "ab ab ab ab ab");
}

TEST(justified_rtl_paragraph_with_digits) {
    assertTwins(true, HE " " HE " 12 " HE " " HE, HE " " HE " " HE " " HE " " HE);
}

TEST(justified_run_width_includes_remainder_pixels) {
    // Five one-glyph words in 78px: 6px of slack over four gaps, so the
    // first two gaps, both in the leading Hebrew run, take an extra pixel.
    assertTwins(true, "\xD7\xA9 \xD7\xA9 a \xD7\xA9 \xD7\xA9 \xD7\xA9",
                "\xD7\xA9 \xD7\xA9 \xD7\xA9 \xD7\xA9 \xD7\xA9 \xD7\xA9", 78);
}

// --- Trailing hyphen ---
// "שש abcd|efghijk": the Latin word breaks at its soft hyphen. In the RTL
// paragraph, "abcd-" sits at the left end with the hyphen between the
// fragment and the Hebrew: abcd 0..31, hyphen 32..39, space, Hebrew 48..63.

static void assertRtlHyphenAfterLatinFragment(bool textView) {
    auto d = draw(textView, HE " abcd" SHY "efghijk", 80, TextAlignment::Left);
    ASSERT_EQ(d->pixel(0, 2), 1);       // 'a' is a full-height letter
    ASSERT_EQ(d->pixel(31, 2), 1);
    ASSERT_EQ(d->pixel(35, 2), 0xFF);   // hyphen: stroke only
    ASSERT_EQ(d->pixel(35, 6), 1);
    ASSERT_EQ(d->pixel(44, 2), 0xFF);   // space
    ASSERT_EQ(d->pixel(48, 2), 1);      // Hebrew
    ASSERT_EQ(d->pixel(63, 2), 1);
    ASSERT_EQ(d->pixel(64, 2), 0xFF);
}

TEST(rtl_trailing_hyphen_follows_latin_fragment_label) {
    assertRtlHyphenAfterLatinFragment(false);
}

TEST(rtl_trailing_hyphen_follows_latin_fragment_text_view) {
    assertRtlHyphenAfterLatinFragment(true);
}

TEST(rtl_trailing_hyphen_reserved_at_left_when_right_aligned) {
    // Right alignment: the Hebrew sits flush right at 72..79, and the
    // hyphen's room is taken at the left end, not the right.
    auto d = draw(true, HE " abcd" SHY "efghijk", 80, TextAlignment::Right);
    ASSERT_EQ(d->pixel(79, 2), 1);
    ASSERT_EQ(d->pixel(64, 2), 1);
    ASSERT_EQ(d->pixel(51, 6), 1);      // hyphen at 48..55
    ASSERT_EQ(d->pixel(51, 2), 0xFF);
    ASSERT_EQ(d->pixel(16, 2), 1);      // "abcd" at 16..47
    ASSERT_EQ(d->pixel(12, 2), 0xFF);
}

TEST(ltr_trailing_hyphen_unchanged_when_justified) {
    // "ab cd ef|gh": the hyphen still ends the line at the right edge.
    auto d = draw(true, "ab cd ef" SHY "ghijklmnop", 80, TextAlignment::Justified);
    ASSERT_EQ(d->pixel(0, 2), 1);
    ASSERT_EQ(d->pixel(75, 6), 1);      // hyphen at 72..79
    ASSERT_EQ(d->pixel(75, 2), 0xFF);
    ASSERT_EQ(d->pixel(71, 2), 1);      // 'f' ends at 71
}
