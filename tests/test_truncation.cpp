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
#include <utility>
#include <vector>

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

TEST(truncation_tail_stops_at_paragraph_break) {
    // "aa\nzz" in a one-line label: the truncated line is "aa…" — the
    // walk must not pull text from past the newline onto this line.
    // Solid font: the ellipsis drawn ON this line (x=16..23) is the
    // discriminator — a walk that runs past the newline would push the
    // ellipsis onto a second, clipped line, leaving x=16..23 dark.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aa\nzz", 40, 12, makeSolidFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(8, 5), 1);      // second 'a'
    ASSERT_EQ(display->pixel(17, 5), 1);     // ellipsis on this line, x=16..23
    ASSERT_EQ(display->pixel(30, 5), 0xFF);  // nothing from past the newline
}

TEST(truncation_tail_trims_space_before_ellipsis) {
    // Width 48, budget 40: the walk keeps "aaaa " (40px), then the trim
    // drops the dangling space, so the run is "aaaa" + ellipsis at
    // x=32..39 (blank cell). Without the trim, the space's solid glyph
    // would light x=32..39.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa aaaa", 48, 12, makeBlankEllipsisFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(31, 5), 1);     // prefix "aaaa"
    ASSERT_EQ(display->pixel(35, 5), 0xFF);  // ellipsis cell where the space would be
    ASSERT_EQ(display->pixel(44, 5), 0xFF);  // untouched beyond the ellipsis
}

TEST(truncation_tail_frame_narrower_than_ellipsis) {
    // 4px frame: no prefix fits; the ellipsis renders alone and clips.
    // Solid font so the ellipsis is visible; must not crash.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa", 4, 12, makeSolidFont());
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(0, 5), 1);      // clipped ellipsis pixels
    ASSERT_EQ(display->pixel(3, 5), 1);
}

// Provider without U+2026 whose '.' glyph is blank: under the "..."
// fallback the dots' cells stay background, revealing which ellipsis
// path ran (U+2026 would leave x=16..31 solid prefix instead).
class NoEllipsisGlyphProvider : public MockGlyphProvider {
public:
    NoEllipsisGlyphProvider() : MockGlyphProvider(8, 12) {
        memset(this->bitmap, 0xFF, sizeof(this->bitmap));
        this->limitedGlyphSet = {'a', ' ', '.'};  // hasGlyph(0x2026) == false
    }
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint,
                                     focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        if (codepoint == '.') return this->blank;
        return MockGlyphProvider::glyphForCodepoint(codepoint, emphasis);
    }
private:
    uint8_t blank[32 * 6] = {};
};

TEST(truncation_tail_falls_back_to_three_periods) {
    // No U+2026 in the face: the ellipsis is "..." (24px), budget 16,
    // so only "aa" survives and x=16 onward is the blank dots region.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaa aaaa", 40, 12,
                                  Font::withProvider(std::make_shared<NoEllipsisGlyphProvider>()));
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(8, 5), 1);      // prefix "aa"
    ASSERT_EQ(display->pixel(15, 5), 1);
    ASSERT_EQ(display->pixel(20, 5), 0xFF);  // dots region, not prefix glyphs
}

TEST(truncation_tail_centers_prefix_and_ellipsis_as_one_unit) {
    // Width 60: line one force-breaks after 8 glyphs (64px) with text
    // still to come, so it truncates — prefix 6 glyphs + ellipsis = 56px,
    // slack 4, so the truncated line is centered at x=2..57.
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aaaaaaaa aaaa", 60, 12, makeSolidFont());
    label->setTruncationMode(TruncationMode::Tail);
    label->setTextAlignment(TextAlignment::Center);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(display->pixel(1, 5), 0xFF);   // left slack
    ASSERT_EQ(display->pixel(2, 5), 1);      // centered run starts
    ASSERT_EQ(display->pixel(57, 5), 1);     // centered run ends
    ASSERT_EQ(display->pixel(58, 5), 0xFF);  // right slack
}

// Provider that records the emphasis each glyph was requested with.
class EmphasisRecordingProvider : public MockGlyphProvider {
public:
    EmphasisRecordingProvider() : MockGlyphProvider(8, 12) {
        memset(this->bitmap, 0xFF, sizeof(this->bitmap));
    }
    bool supportsEmphasis(focus::FontStyle emphasis) const override { return true; }
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint,
                                     focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        this->drawn.push_back({codepoint, emphasis});
        return MockGlyphProvider::glyphForCodepoint(codepoint, emphasis);
    }
    mutable std::vector<std::pair<UNICODE_CODEPOINT, focus::FontStyle>> drawn;
};

TEST(truncation_tail_ellipsis_inherits_emphasis_at_cut) {
    // "aa" then SO (italic on) then "bbbbbb": the cut lands inside the
    // italic run, so the ellipsis must be requested at Italic.
    auto provider = std::make_shared<EmphasisRecordingProvider>();
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<LabelView> label;
    auto window = makeLabelWindow(display, label, "aa\x0E" "bbbbbb", 40, 12,
                                  Font::withProvider(provider));
    label->setTruncationMode(TruncationMode::Tail);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    bool sawItalicEllipsis = false;
    for (const auto& entry : provider->drawn) {
        if (entry.first == 0x2026 && entry.second == FontStyle::Italic) sawItalicEllipsis = true;
    }
    ASSERT_TRUE(sawItalicEllipsis);
}

