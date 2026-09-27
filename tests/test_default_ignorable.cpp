/*
 * Default-ignorable code points (U+200B, U+FEFF, direction marks, ...)
 * are zero-width in measurement and paint nothing. The mock gives every
 * code point a full 8px advance and a solid glyph, as Unifont's
 * replacement glyph does for code points it lacks.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "MockGlyphProvider.hpp"
#include "Window.hpp"
#include "LabelView.hpp"
#include "TextView.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <cstring>
#include <string>
#include <vector>

using namespace focus;

static std::shared_ptr<MockGlyphProvider> makeSolidProvider() {
    auto provider = std::make_shared<MockGlyphProvider>(8, 12);
    memset(provider->bitmap, 0xFF, sizeof(provider->bitmap));
    return provider;
}

// Inked columns in row 5 of a label drawn at the top left of an 80px window.
static int inkedColumns(const char* text) {
    auto display = std::make_shared<RecordingDisplay>(80, 12);
    auto window = std::make_shared<Window>(display, MakeSize(80, 12));
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, 80, 12), text);
    label->setFont(Font::withProvider(makeSolidProvider()));
    window->addSubview(label);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 12));
    int count = 0;
    for (int x = 0; x < 80; x++) {
        if (display->pixel(x, 5) == 1) count++;
    }
    return count;
}

TEST(default_ignorables_measure_zero_width) {
    auto provider = makeSolidProvider();
    const char* ignorables[] = {
        "\xE2\x80\x8B",  // U+200B ZERO WIDTH SPACE
        "\xE2\x80\x8C",  // U+200C ZERO WIDTH NON-JOINER
        "\xE2\x80\x8D",  // U+200D ZERO WIDTH JOINER
        "\xE2\x80\x8E",  // U+200E LEFT-TO-RIGHT MARK
        "\xE2\x80\x8F",  // U+200F RIGHT-TO-LEFT MARK
        "\xE2\x81\xA0",  // U+2060 WORD JOINER
        "\xEF\xBB\xBF",  // U+FEFF ZERO WIDTH NO-BREAK SPACE
        "\xEF\xB8\x8F",  // U+FE0F VARIATION SELECTOR-16
    };
    for (const char* ignorable : ignorables) {
        std::string text = std::string("a") + ignorable + "b";
        ASSERT_EQ(TextLayout::measureTextWidth(text.c_str(), 1, provider.get()), (int16_t)16);
    }
}

TEST(lam_alef_draws_no_box_for_consumed_alef) {
    // "بلا": beh + lam-alef ligature, two visible glyphs.
    ASSERT_EQ(inkedColumns("\xD8\xA8\xD9\x84\xD8\xA7"), 16);
}

TEST(label_of_only_byte_order_mark_draws_nothing) {
    ASSERT_EQ(inkedColumns("\xEF\xBB\xBF"), 0);
}

TEST(ignorable_only_line_counts_as_blank_in_height) {
    auto provider = makeSolidProvider();
    ASSERT_EQ(TextLayout::measureTextHeight("a\n\xEF\xBB\xBF\nb", 80, 1, provider.get()),
              TextLayout::measureTextHeight("a\n\nb", 80, 1, provider.get()));
}

TEST(ignorable_takes_no_room_in_line_wrap) {
    // "abcd e" is exactly 48px; the U+200B must not push "e" to a second line.
    auto provider = makeSolidProvider();
    ASSERT_EQ(TextLayout::measureTextHeight("a\xE2\x80\x8B" "bcd e", 48, 1, provider.get()),
              TextLayout::measureTextHeight("abcd e", 48, 1, provider.get()));
}

// A TextView line holding only U+FEFF gets blank-line spacing, like an empty one.
static std::vector<uint8_t> textViewPixels(const char* text) {
    auto display = std::make_shared<RecordingDisplay>(80, 80);
    auto window = std::make_shared<Window>(display, MakeSize(80, 80));
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, 80, 80), text);
    tv->setFont(Font::withProvider(makeSolidProvider()));
    window->addSubview(tv);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 80));
    return display->framebuffer;
}

TEST(text_view_ignorable_only_line_is_blank) {
    ASSERT_TRUE(textViewPixels("a\n\xEF\xBB\xBF\nb") == textViewPixels("a\n\nb"));
}

// Soft hyphens are default-ignorable too: the U+00AD itself never paints,
// and only the synthesized hyphen at a break takes room.

static std::shared_ptr<RecordingDisplay> drawLabel(const char* text, int width,
                                                   TextAlignment alignment) {
    auto display = std::make_shared<RecordingDisplay>(80, 40);
    auto window = std::make_shared<Window>(display, MakeSize(80, 40));
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, width, 40), text);
    label->setFont(Font::withProvider(makeSolidProvider()));
    label->setTextAlignment(alignment);
    window->addSubview(label);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 80, 40));
    return display;
}

TEST(soft_hyphen_mid_line_paints_nothing) {
    ASSERT_EQ(inkedColumns("ab\xC2\xAD" "cd"), 32);
}

TEST(soft_hyphen_at_break_paints_only_the_hyphen) {
    // 48px: "aaaa" + synthesized hyphen fill 0..39; the U+00AD adds nothing.
    auto display = drawLabel("aaaa\xC2\xADzzzz", 48, TextAlignment::Left);
    ASSERT_EQ(display->pixel(39, 5), 1);
    ASSERT_EQ(display->pixel(40, 5), 0xFF);
}

TEST(soft_hyphen_at_break_takes_no_room_in_alignment) {
    // Right-aligned in 48px: "aaaa" + hyphen is 40px, so ink starts at x=8.
    auto display = drawLabel("aaaa\xC2\xADzzzz", 48, TextAlignment::Right);
    ASSERT_EQ(display->pixel(7, 5), 0xFF);
    ASSERT_EQ(display->pixel(8, 5), 1);
    ASSERT_EQ(display->pixel(47, 5), 1);
}
