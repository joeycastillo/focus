/*
 * Tests for TextView: lazy line index, heightForWidth, visible-line
 * rendering, and parity with LabelView.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "MockGlyphProvider.hpp"
#include "Window.hpp"
#include "TextView.hpp"
#include "TextLayout.hpp"
#include "Font.hpp"
#include "ScrollView.hpp"
#include "LabelView.hpp"

using namespace focus;

// Mock glyphs are 8px advance, 12 rows; lineSpacing 2 -> line pitch 14;
// paragraphSpacing 12/3 = 4 -> paragraph pitch 16.
static std::shared_ptr<Font> makeMockFont() {
    auto provider = std::make_shared<MockGlyphProvider>(8, 12);
    memset(provider->bitmap, 0xFF, sizeof(provider->bitmap));  // solid glyphs
    return Font::withProvider(provider);
}

TEST(text_view_height_for_width_matches_measure_text_height) {
    auto font = makeMockFont();
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, 40, 0), "aaaa bbbb cccc");
    tv->setFont(font);

    int h = tv->heightForWidth(40);
    ASSERT_EQ(h, (int)TextLayout::measureTextHeight("aaaa bbbb cccc", 40, 1,
                                                    font->getGlyphProvider()));
    // 40px fits 5 glyphs per line: three lines, two with trailing spacing.
    ASSERT_EQ(h, 14 + 14 + 12);
}

TEST(text_view_empty_text_measures_zero) {
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, 40, 0), "");
    tv->setFont(makeMockFont());
    ASSERT_EQ(tv->heightForWidth(40), 0);
}

static std::shared_ptr<Window> makeTextWindow(std::shared_ptr<RecordingDisplay>& display,
                                              std::shared_ptr<TextView>& tv,
                                              const char* text, int width) {
    display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    tv = std::make_shared<TextView>(MakeRect(0, 0, width, 0), text);
    tv->setFont(makeMockFont());
    tv->setFrame(MakeRect(0, 0, width, tv->heightForWidth(width)));
    window->addSubview(tv);
    display->reset();
    return window;
}

static int blitCount(const RecordingDisplay& d) {
    int n = 0;
    for (const auto& op : d.ops)
        if (op.type == RecordingDisplay::Op::Type::BlitMasked) n++;
    return n;
}

TEST(text_view_blits_one_mask_per_line_at_indexed_y) {
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<TextView> tv;
    // 40px = 5 mock glyphs per line: "aaaa " / "bbbb " / "cccc" = 3 lines at y 0/14/28.
    auto window = makeTextWindow(display, tv, "aaaa bbbb cccc", 40);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(blitCount(*display), 3);
    int expectedY[3] = {0, 14, 28};
    int i = 0;
    for (const auto& op : display->ops) {
        if (op.type != RecordingDisplay::Op::Type::BlitMasked) continue;
        ASSERT_EQ(op.y, expectedY[i]);
        ASSERT_EQ(op.h, 12);  // glyph rows only; spacing stays background
        i++;
    }
}

TEST(text_view_renders_only_clip_visible_lines) {
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<TextView> tv;
    auto window = makeTextWindow(display, tv, "aaaa bbbb cccc", 40);
    window->draw(0, 0, MakeRect(0, 14, 40, 12));  // second line's band only

    ASSERT_EQ(blitCount(*display), 1);
    for (const auto& op : display->ops)
        if (op.type == RecordingDisplay::Op::Type::BlitMasked) ASSERT_EQ(op.y, 14);
}

TEST(text_view_soft_hyphen_renders_trailing_hyphen) {
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<TextView> tv;
    // "aaaa­bbbb" at 40px: breaks at the soft hyphen, hyphen glyph drawn
    // at the end of line 0 — with solid mock glyphs, pixels reach column 39.
    auto window = makeTextWindow(display, tv, "aaaa\xC2\xADzzzz", 40);
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    ASSERT_EQ(blitCount(*display), 2);
    ASSERT_EQ(display->pixel(39, 5), 1);   // hyphen occupies the reserved slot
    ASSERT_EQ(display->pixel(0, 19), 1);   // second line starts at x=0, y=14..26
}

TEST(text_view_inside_scroll_view_shows_correct_band) {
    auto display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    // Viewport shows one line-pitch worth of a three-line text.
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 100, 40, 14));
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, 40, 0), "aaaa bbbb cccc");
    tv->setFont(makeMockFont());
    int h = tv->heightForWidth(40);
    tv->setFrame(MakeRect(0, 0, 40, h));
    sv->setContentSize(MakeSize(40, h));
    window->addSubview(sv);
    sv->addSubview(tv);
    sv->setScrollOffset(MakePoint(0, 14));  // scroll to the second line
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    // Only the second line is rasterized, blitted at the viewport top.
    ASSERT_EQ(blitCount(*display), 1);
    for (const auto& op : display->ops)
        if (op.type == RecordingDisplay::Op::Type::BlitMasked) ASSERT_EQ(op.y, 100);
    ASSERT_EQ(display->pixel(0, 105), 1);   // glyph pixels inside the viewport
    ASSERT_EQ(display->pixel(0, 99), 0xFF); // nothing above it
}

namespace {
// A mock provider that claims real bold/italic glyphs, so any synthetic
// smear/shear applied on top of it is a bug, not a compensating fallback.
class EmphasisCapableMockProvider : public MockGlyphProvider {
public:
    using MockGlyphProvider::MockGlyphProvider;
    bool supportsEmphasis(focus::FontStyle) const override { return true; }
};
}  // namespace

// TextView's LineScratch must inherit the view's font (as LabelView's canvas
// does), or CanvasView::drawGlyph sees a null font and synthesizes bold/italic
// on top of glyphs the provider already renders natively.
TEST(text_view_scratch_inherits_font_for_emphasis) {
    auto provider = std::make_shared<EmphasisCapableMockProvider>(8, 12);
    memset(provider->bitmap, 0xFF, sizeof(provider->bitmap));  // solid glyphs
    auto font = Font::withProvider(provider);

    const char* text = "\x0E\x0E" "aaaa";  // two SO codes -> emphasis depth 2 (bold)
    const int width = 100;
    const int rowHeight = provider->getGlyphRowCount();

    auto labelDisplay = std::make_shared<RecordingDisplay>(width, rowHeight);
    auto labelWindow = std::make_shared<Window>(labelDisplay, MakeSize(width, rowHeight));
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, width, rowHeight), text);
    label->setFont(font);
    labelWindow->addSubview(label);
    labelDisplay->reset();
    labelWindow->draw(0, 0, MakeRect(0, 0, width, rowHeight));

    auto textDisplay = std::make_shared<RecordingDisplay>(width, rowHeight);
    auto textWindow = std::make_shared<Window>(textDisplay, MakeSize(width, rowHeight));
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, width, 0), text);
    tv->setFont(font);
    tv->setFrame(MakeRect(0, 0, width, tv->heightForWidth(width)));
    textWindow->addSubview(tv);
    textDisplay->reset();
    textWindow->draw(0, 0, MakeRect(0, 0, width, rowHeight));

    ASSERT_EQ(labelDisplay->framebuffer.size(), textDisplay->framebuffer.size());
    bool same = labelDisplay->framebuffer == textDisplay->framebuffer;
    ASSERT_TRUE(same);
}

// Render the same text through LabelView and TextView into two displays and
// compare every pixel. Single-paragraph inputs only: multi-paragraph vertical
// advance is asserted against the measurement layer in the earlier tests, not
// against LabelView (see the design spec).
static void assertParity(const char* text, int width, TextAlignment alignment) {
    auto font = makeMockFont();

    std::shared_ptr<RecordingDisplay> labelDisplay = std::make_shared<RecordingDisplay>(480, 800);
    auto labelWindow = std::make_shared<Window>(labelDisplay, MakeSize(480, 800));
    auto label = std::make_shared<LabelView>(MakeRect(0, 0, width, 400), text);
    label->setFont(font);
    label->setTextAlignment(alignment);
    labelWindow->addSubview(label);
    labelDisplay->reset();
    labelWindow->draw(0, 0, MakeRect(0, 0, 480, 800));

    std::shared_ptr<RecordingDisplay> textDisplay = std::make_shared<RecordingDisplay>(480, 800);
    auto textWindow = std::make_shared<Window>(textDisplay, MakeSize(480, 800));
    auto tv = std::make_shared<TextView>(MakeRect(0, 0, width, 400), text);
    tv->setFont(font);
    tv->setTextAlignment(alignment);
    textWindow->addSubview(tv);
    textDisplay->reset();
    textWindow->draw(0, 0, MakeRect(0, 0, 480, 800));

    // Guard against vacuous parity: the frames must contain real glyph pixels.
    ASSERT_TRUE(std::count(textDisplay->framebuffer.begin(),
                           textDisplay->framebuffer.end(), (uint8_t)1) > 0);

    ASSERT_TRUE(labelDisplay->framebuffer == textDisplay->framebuffer);
}

TEST(text_view_parity_wrapped_ltr) {
    assertParity("aaaa bbbb cccc dddd eee", 40, TextAlignment::Left);
}

TEST(text_view_parity_soft_hyphen) {
    assertParity("aaaa\xC2\xADzzzz qq", 40, TextAlignment::Left);
}

TEST(text_view_parity_center_alignment) {
    assertParity("aa bbbb c", 40, TextAlignment::Center);
}

TEST(text_view_parity_right_alignment) {
    assertParity("aa bbbb c", 40, TextAlignment::Right);
}

TEST(text_view_parity_emphasis_codes) {
    // SO ... SI emphasis crossing a line break: depth must carry to line 2.
    assertParity("aa \x0E" "bbbb cccc\x0F dd", 40, TextAlignment::Left);
}

TEST(text_view_parity_rtl_runs) {
    // Arabic runs exercise bidi reordering and shaping inside the LTR paragraph.
    assertParity("aa \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7 bb", 40, TextAlignment::Left);
}
