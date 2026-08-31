/*
 * Tests for TruncationMode: tail truncation with an ellipsis in
 * CanvasView's text pipeline, driven through LabelView.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "MockGlyphProvider.hpp"
#include "Window.hpp"
#include "LabelView.hpp"
#include "Font.hpp"

using namespace focus;

// Mock glyphs are 8px advance, 12 rows; line pitch 14, paragraph pitch 16.
// 40px wide fits 5 glyphs per line.
static std::shared_ptr<Font> makeSolidFont() {
    auto provider = std::make_shared<MockGlyphProvider>(8, 12);
    memset(provider->bitmap, 0xFF, sizeof(provider->bitmap));  // solid glyphs
    return Font::withProvider(provider);
}

// Provider whose ellipsis glyph is blank: the ellipsis's cell stays
// background, making the cut point visible against solid glyphs.
class BlankEllipsisProvider : public MockGlyphProvider {
public:
    using MockGlyphProvider::MockGlyphProvider;
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint,
                                     focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        if (codepoint == 0x2026) return this->blank;
        return MockGlyphProvider::glyphForCodepoint(codepoint, emphasis);
    }
private:
    uint8_t blank[32 * 6] = {};
};

static std::shared_ptr<Font> makeBlankEllipsisFont() {
    auto provider = std::make_shared<BlankEllipsisProvider>(8, 12);
    memset(provider->bitmap, 0xFF, sizeof(provider->bitmap));
    return Font::withProvider(provider);
}

static std::shared_ptr<Window> makeLabelWindow(std::shared_ptr<RecordingDisplay>& display,
                                               std::shared_ptr<LabelView>& label,
                                               const char* text, int width, int height,
                                               std::shared_ptr<Font> font) {
    display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    label = std::make_shared<LabelView>(MakeRect(0, 0, width, height), text);
    label->setFont(font);
    window->addSubview(label);
    display->reset();
    return window;
}

TEST(truncation_mode_defaults_to_none) {
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, 40, 12), "aaaa");
    ASSERT_TRUE(label->getTruncationMode() == TruncationMode::None);
    label->setTruncationMode(TruncationMode::Tail);
    ASSERT_TRUE(label->getTruncationMode() == TruncationMode::Tail);
}

TEST(truncation_none_renders_exactly_as_before) {
    // Regression lock: default mode fills two lines and clips the third,
    // exactly as today. Line 1's fifth glyph (a solid space) reaches x=39.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa aaaa aaaa", 40, 26, makeSolidFont());
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(0, 5), 1);      // line 0 starts at x=0
    ASSERT_EQ(display->pixel(39, 5), 1);     // line 0 fills the width
    ASSERT_EQ(display->pixel(0, 19), 1);     // line 1 at y=14..25
    ASSERT_EQ(display->pixel(32, 19), 1);    // line 1's fifth glyph is drawn
    ASSERT_EQ(display->pixel(0, 27), 0xFF);  // nothing below the frame
}

TEST(truncation_tail_single_line) {
    // One-line label, overflowing text: 4 glyphs fit beside the 8px
    // ellipsis (budget 40-8=32), so glyph five's cell stays background.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa aaaa", 40, 12, makeBlankEllipsisFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(0, 5), 1);      // prefix starts at x=0
    ASSERT_EQ(display->pixel(31, 5), 1);     // prefix runs through glyph 4
    ASSERT_EQ(display->pixel(32, 5), 0xFF);  // ellipsis cell (blank glyph)
    ASSERT_EQ(display->pixel(39, 5), 0xFF);
}

TEST(truncation_tail_second_line_of_two) {
    // Two lines fit (y=0 and y=14 in a 26px frame). Line 0 wraps
    // normally; line 1 is the last fit with text remaining, so it
    // truncates: 4 glyphs + ellipsis, cell five stays background.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa aaaa aaaa", 40, 26, makeBlankEllipsisFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(0, 5), 1);      // line 0 unchanged
    ASSERT_EQ(display->pixel(39, 5), 1);     // line 0 still fills the width
    ASSERT_EQ(display->pixel(0, 19), 1);     // line 1 prefix
    ASSERT_EQ(display->pixel(31, 19), 1);
    ASSERT_EQ(display->pixel(32, 19), 0xFF); // line 1 ellipsis cell
}

TEST(truncation_tail_exact_fit_draws_no_ellipsis) {
    // Text that fits renders identically to None: no ellipsis appears.
    // Solid font here — a spurious ellipsis would light x=32..39.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa", 40, 12, makeSolidFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(31, 5), 1);     // all four glyphs drawn
    ASSERT_EQ(display->pixel(33, 5), 0xFF);  // nothing appended after them
}
