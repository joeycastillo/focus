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
