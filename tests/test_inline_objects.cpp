/*
 * Inline objects: U+FFFC takes a caller-supplied width, is broken around
 * like an atomic inline, and reports where it lands.
 */

#include "test_harness.hpp"
#include "MockGlyphProvider.hpp"
#include "TextLayout.hpp"
#include "InlineObjectProvider.hpp"
#include "Utf8.hpp"
#include "CanvasView.hpp"
#include "Font.hpp"
#include "ArabicShaping.hpp"
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

using namespace focus;

namespace {

// Every object has the same width; placements are recorded by index.
struct FixedObjects : public InlineObjectProvider {
    const UNICODE_CODEPOINT* base;
    int16_t width;
    mutable int widthCalls = 0;
    std::vector<std::pair<size_t, int16_t>> placed;

    FixedObjects(const UNICODE_CODEPOINT* base, int16_t width) : base(base), width(width) {}

    int16_t widthOfObject(const UNICODE_CODEPOINT* object) const override {
        widthCalls++;
        return this->width;
    }
    void objectPlaced(const UNICODE_CODEPOINT* object, int16_t x) override {
        placed.push_back({(size_t)(object - base), x});
    }
};

}  // namespace

static std::vector<UNICODE_CODEPOINT> cps(const char* utf8) {
    std::vector<UNICODE_CODEPOINT> v(utf8_codepoint_length(utf8));
    utf8_parse(utf8, v.data());
    return v;
}

#define OBJ "\xEF\xBF\xBC"
#define WJ "\xE2\x81\xA0"
#define NBSP "\xC2\xA0"

static WordWrapResult wrap(std::vector<UNICODE_CODEPOINT>& text, size_t offset,
                           int16_t width, int16_t objectWidth,
                           const Hyphenator* hyphenator = nullptr) {
    MockGlyphProvider gp(8);
    FixedObjects objects(text.data(), objectWidth);
    return TextLayout::measureLineWrap(text.data() + offset, text.size() - offset, width, 1, &gp,
                                       0, FontStyle::Regular, hyphenator, &objects);
}

// --- Measurement ---

TEST(inline_object_measured_at_its_width) {
    // 32 + 40 + 32 = 104 overflows 100: break after the object.
    auto text = cps("abcd" OBJ "efgh");
    WordWrapResult r = wrap(text, 0, 100, 40);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)5);
    // 32 + 16 + 32 = 80 fits.
    r = wrap(text, 0, 100, 16);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)80);
}

TEST(inline_object_wider_than_255) {
    auto text = cps(OBJ);
    WordWrapResult r = wrap(text, 0, 1000, 300);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);
    ASSERT_EQ(r.endCursorX, (int16_t)300);
}

TEST(inline_object_wider_than_line_takes_own_line) {
    auto text = cps("ab " OBJ " cd");
    WordWrapResult r = wrap(text, 0, 100, 300);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);
    r = wrap(text, 3, 100, 300);
    ASSERT_TRUE(r.wrapped);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)1);
}

TEST(inline_object_emphasis_codes_leave_width_alone) {
    auto text = cps("a\x0E" OBJ "\x0F" "b");
    WordWrapResult r = wrap(text, 0, 100, 20);
    ASSERT_EQ(r.endCursorX, (int16_t)36);
}

TEST(inline_object_without_provider_is_a_glyph) {
    auto text = cps("ab" OBJ "cd");
    MockGlyphProvider gp(8);
    WordWrapResult r = TextLayout::measureLineWrap(text.data(), text.size(), 100, 1, &gp);
    ASSERT_EQ(r.endCursorX, (int16_t)40);
}

// --- Breaks ---

TEST(inline_object_breaks_before) {
    // 32 + 80 overflows 100: break between the word and the object.
    auto text = cps("abcd" OBJ);
    WordWrapResult r = wrap(text, 0, 100, 80);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)4);
}

TEST(inline_object_breaks_before_at_second_code_point) {
    // The break after "a" is candidate 0.
    auto text = cps("a" OBJ);
    WordWrapResult r = wrap(text, 0, 80, 80);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)1);
}

TEST(adjacent_inline_objects_break_between) {
    auto text = cps(OBJ OBJ);
    WordWrapResult r = wrap(text, 0, 80, 48);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)1);
}

TEST(word_joiner_glues_word_to_following_object) {
    // No break between "cd" and the object: the line breaks at the space.
    auto text = cps("ab cd" WJ OBJ);
    WordWrapResult r = wrap(text, 0, 80, 48);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);
}

TEST(no_break_space_does_not_glue_object) {
    auto text = cps("ab cd" NBSP OBJ);
    WordWrapResult r = wrap(text, 0, 80, 48);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)6);
}

TEST(word_joiner_glues_object_to_following_word) {
    // No break after the object: the line breaks before it, at the space.
    auto text = cps("ab " OBJ WJ "cdefgh");
    WordWrapResult r = wrap(text, 0, 80, 16);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)3);
}

// --- Hyphenation ---

namespace {

// Records the word it was offered; offers no positions.
struct CapturingHyphenator : public Hyphenator {
    mutable int calls = 0;
    mutable size_t wordLen = 0;
    mutable bool sawObject = false;
    size_t findBreakPositions(const UNICODE_CODEPOINT* w, size_t n,
                              size_t*, size_t) const override {
        calls++;
        wordLen = n;
        for (size_t i = 0; i < n; i++) {
            if (w[i] == TextControlCode::ObjectReplacement) sawObject = true;
        }
        return 0;
    }
};

// Offers one break, after the fourth code point of any word.
struct AfterFourthHyphenator : public Hyphenator {
    size_t findBreakPositions(const UNICODE_CODEPOINT*, size_t n,
                              size_t* positions, size_t max) const override {
        if (n < 5 || max < 1) return 0;
        positions[0] = 3;
        return 1;
    }
};

}  // namespace

TEST(hyphenated_word_ends_at_inline_object) {
    auto text = cps("abcdefghij" OBJ);
    CapturingHyphenator h;
    wrap(text, 0, 48, 8, &h);
    ASSERT_EQ(h.calls, 1);
    ASSERT_EQ(h.wordLen, (size_t)10);
    ASSERT_FALSE(h.sawObject);
}

TEST(hyphen_prefix_counts_inline_object_width) {
    // Object 24 + space 8 + "abcd" 32 + hyphen 8 = 72 > 64: no hyphen.
    auto text = cps(OBJ " abcdefgh");
    AfterFourthHyphenator h;
    WordWrapResult r = wrap(text, 0, 64, 24, &h);
    ASSERT_FALSE(r.needsHyphen);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)2);
}

// --- Rendering ---

namespace {

// Solid 8px glyphs, spaces included, so every gap in the ink is an object
// or justification.
class SolidProvider : public MockGlyphProvider {
public:
    SolidProvider() : MockGlyphProvider(8, 12) {
        memset(this->bitmap, 0xFF, sizeof(this->bitmap));
    }
};

// Lays out one line through renderBidiLine, as a subclass would.
class ObjectCanvas : public CanvasView {
public:
    explicit ObjectCanvas(int width) : CanvasView(MakeRect(0, 0, width, 12)) {}
    using CanvasView::inlineObjects;

    void renderLine(std::vector<UNICODE_CODEPOINT>& text, TextAlignment alignment,
                    InlineObjectProvider* objects, int paragraphDir = 0) {
        this->clear(0);
        this->textSize = 1;
        this->textColor = 1;
        this->textLayoutRect = MakeRect(0, 0, this->getCanvasWidth(), this->getCanvasHeight());
        this->cursor = this->textLayoutRect.origin;
        this->textAlignment = alignment;
        this->emphasisDepth = 0;
        this->glyphRowCount = this->provider.getGlyphRowCount();
        if (paragraphDir == 0) {
            paragraphDir = TextLayout::paragraphDirection(text.data(), text.size());
        }
        this->inlineObjects = objects;
        this->renderBidiLine(text.data(), 0, text.size(), paragraphDir,
                             (int16_t)this->getCanvasWidth(), 0, &this->provider);
        this->inlineObjects = nullptr;
    }

    bool ink(int x) const {
        if (x < 0 || x >= this->getCanvasWidth()) return false;
        const uint8_t* row = this->getBufferData() + 5 * this->getRowBytes();
        return (row[x / 8] >> (7 - x % 8)) & 1;
    }
    bool inkFrom(int from, int to) const {
        for (int x = from; x <= to; x++) if (!this->ink(x)) return false;
        return true;
    }
    bool blankFrom(int from, int to) const {
        for (int x = from; x <= to; x++) if (this->ink(x)) return false;
        return true;
    }
    int inkExtent() const {
        int last = -1;
        for (int x = 0; x < this->getCanvasWidth(); x++) if (this->ink(x)) last = x;
        return last + 1;
    }

    SolidProvider provider;
};

}  // namespace

#define ALEF "\xD7\x90"
#define BET "\xD7\x91"
#define GIMEL "\xD7\x92"
#define DALET "\xD7\x93"

TEST(inline_object_placed_ltr) {
    auto text = cps("ab" OBJ "cd");
    FixedObjects objects(text.data(), 40);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Left, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].first, (size_t)2);
    ASSERT_EQ(objects.placed[0].second, (int16_t)16);
    ASSERT_TRUE(canvas.inkFrom(0, 15));
    ASSERT_TRUE(canvas.blankFrom(16, 55));
    ASSERT_TRUE(canvas.inkFrom(56, 71));
    ASSERT_FALSE(canvas.ink(72));
}

TEST(inline_object_placed_rtl) {
    // From the right: alef bet 84..99, object 44..83, gimel dalet 28..43.
    auto text = cps(ALEF BET OBJ GIMEL DALET);
    FixedObjects objects(text.data(), 40);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Right, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].first, (size_t)2);
    ASSERT_EQ(objects.placed[0].second, (int16_t)44);
    ASSERT_TRUE(canvas.inkFrom(84, 99));
    ASSERT_TRUE(canvas.blankFrom(44, 83));
    ASSERT_TRUE(canvas.inkFrom(28, 43));
    ASSERT_FALSE(canvas.ink(27));
}

TEST(two_inline_objects_placed_rtl) {
    auto text = cps(ALEF OBJ BET OBJ GIMEL);
    FixedObjects objects(text.data(), 16);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Right, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)2);
    ASSERT_EQ(objects.placed[0].first, (size_t)1);
    ASSERT_EQ(objects.placed[0].second, (int16_t)76);
    ASSERT_EQ(objects.placed[1].first, (size_t)3);
    ASSERT_EQ(objects.placed[1].second, (int16_t)52);
    ASSERT_TRUE(canvas.blankFrom(76, 91));
    ASSERT_TRUE(canvas.blankFrom(52, 67));
    ASSERT_TRUE(canvas.inkFrom(68, 75));
}

TEST(inline_object_in_latin_run_of_rtl_paragraph) {
    // "ab<obj>cd" is one LTR run placed left of the Hebrew: 28..75.
    auto text = cps(ALEF BET " ab" OBJ "cd");
    FixedObjects objects(text.data(), 16);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Right, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].first, (size_t)5);
    ASSERT_EQ(objects.placed[0].second, (int16_t)44);
    ASSERT_TRUE(canvas.inkFrom(28, 43));
    ASSERT_TRUE(canvas.blankFrom(44, 59));
    ASSERT_TRUE(canvas.inkFrom(60, 75));
}

TEST(inline_object_placed_justified_ltr) {
    // 68px of content, 32px of slack over two gaps.
    auto text = cps("ab " OBJ " cd");
    FixedObjects objects(text.data(), 20);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Justified, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].second, (int16_t)40);
    ASSERT_TRUE(canvas.ink(0));
    ASSERT_TRUE(canvas.ink(99));
    ASSERT_TRUE(canvas.blankFrom(40, 59));
    ASSERT_TRUE(canvas.ink(60));
}

TEST(inline_object_placed_justified_rtl) {
    auto text = cps(ALEF BET " " OBJ " " GIMEL DALET);
    FixedObjects objects(text.data(), 20);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Justified, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].second, (int16_t)40);
    ASSERT_TRUE(canvas.ink(0));
    ASSERT_TRUE(canvas.ink(99));
    ASSERT_TRUE(canvas.blankFrom(40, 59));
    ASSERT_TRUE(canvas.ink(39));
}

TEST(inline_object_at_line_start_right_and_center) {
    auto text = cps(OBJ "ab");
    FixedObjects right(text.data(), 20);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Right, &right);
    ASSERT_EQ(right.placed.size(), (size_t)1);
    ASSERT_EQ(right.placed[0].second, (int16_t)64);
    ASSERT_TRUE(canvas.blankFrom(64, 83));
    FixedObjects center(text.data(), 20);
    canvas.renderLine(text, TextAlignment::Center, &center);
    ASSERT_EQ(center.placed.size(), (size_t)1);
    ASSERT_EQ(center.placed[0].second, (int16_t)32);
    ASSERT_TRUE(canvas.blankFrom(32, 51));
}

TEST(inline_object_at_line_end_right_and_center) {
    // The trailing space a wrapped line keeps doesn't move the object.
    auto text = cps("ab" OBJ " ");
    FixedObjects right(text.data(), 20);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Right, &right);
    ASSERT_EQ(right.placed.size(), (size_t)1);
    ASSERT_EQ(right.placed[0].second, (int16_t)80);
    ASSERT_TRUE(canvas.inkFrom(64, 79));
    FixedObjects center(text.data(), 20);
    canvas.renderLine(text, TextAlignment::Center, &center);
    ASSERT_EQ(center.placed.size(), (size_t)1);
    ASSERT_EQ(center.placed[0].second, (int16_t)48);
    ASSERT_TRUE(canvas.inkFrom(32, 47));
}

TEST(inline_object_wider_than_line_placed_at_start_edge) {
    auto text = cps(OBJ);
    ObjectCanvas canvas(100);
    FixedObjects ltr(text.data(), 300);
    canvas.renderLine(text, TextAlignment::Left, &ltr, 1);
    ASSERT_EQ(ltr.placed.size(), (size_t)1);
    ASSERT_EQ(ltr.placed[0].second, (int16_t)0);
    FixedObjects rtlLeft(text.data(), 300);
    canvas.renderLine(text, TextAlignment::Left, &rtlLeft, -1);
    ASSERT_EQ(rtlLeft.placed.size(), (size_t)1);
    ASSERT_EQ(rtlLeft.placed[0].second, (int16_t)0);
    FixedObjects rtlRight(text.data(), 300);
    canvas.renderLine(text, TextAlignment::Right, &rtlRight, -1);
    ASSERT_EQ(rtlRight.placed.size(), (size_t)1);
    ASSERT_EQ(rtlRight.placed[0].second, (int16_t)-200);
}

TEST(inline_object_emphasis_codes_leave_position_alone) {
    auto text = cps("a\x0E" OBJ "\x0F" "b");
    FixedObjects objects(text.data(), 20);
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Left, &objects);
    ASSERT_EQ(objects.placed.size(), (size_t)1);
    ASSERT_EQ(objects.placed[0].second, (int16_t)8);
    ASSERT_TRUE(canvas.inkFrom(28, 35));
}

TEST(inline_object_measure_and_render_agree) {
    MockGlyphProvider gp(8);
    auto ltr = cps("ab" OBJ "cd");
    FixedObjects ltrObjects(ltr.data(), 40);
    WordWrapResult r = TextLayout::measureLineWrap(ltr.data(), ltr.size(), 100, 1, &gp,
                                                   0, FontStyle::Regular, nullptr, &ltrObjects);
    ObjectCanvas canvas(100);
    canvas.renderLine(ltr, TextAlignment::Left, &ltrObjects);
    ASSERT_EQ(canvas.inkExtent(), (int)r.endCursorX);

    auto rtl = cps(ALEF BET OBJ GIMEL DALET);
    FixedObjects rtlObjects(rtl.data(), 40);
    r = TextLayout::measureLineWrap(rtl.data(), rtl.size(), 100, 1, &gp,
                                    0, FontStyle::Regular, nullptr, &rtlObjects);
    canvas.renderLine(rtl, TextAlignment::Left, &rtlObjects);
    ASSERT_EQ(canvas.inkExtent(), (int)r.endCursorX);
}

TEST(inline_object_without_provider_draws_as_glyph) {
    auto text = cps("ab" OBJ "cd");
    ObjectCanvas canvas(100);
    canvas.renderLine(text, TextAlignment::Left, nullptr);
    ASSERT_TRUE(canvas.inkFrom(0, 39));
    ASSERT_FALSE(canvas.ink(40));
}

TEST(draw_text_ignores_inline_objects) {
    auto text = cps("ab" OBJ "cd");
    FixedObjects objects(text.data(), 40);
    auto font = Font::withProvider(std::make_shared<SolidProvider>());
    ObjectCanvas withMember(100), without(100);
    withMember.setFont(font);
    without.setFont(font);
    withMember.inlineObjects = &objects;
    withMember.drawText(MakeRect(0, 0, 100, 12), 1, 1, "ab" OBJ "cd");
    without.drawText(MakeRect(0, 0, 100, 12), 1, 1, "ab" OBJ "cd");
    ASSERT_EQ(objects.widthCalls, 0);
    ASSERT_TRUE(withMember.inlineObjects == &objects);
    ASSERT_TRUE(memcmp(withMember.getBufferData(), without.getBufferData(),
                       (size_t)withMember.getRowBytes() * 12) == 0);
}

TEST(inline_object_does_not_join_arabic) {
    // Beh, object, beh: both behs take their isolated form.
    auto text = cps("\xD8\xA8" OBJ "\xD8\xA8");
    shapeArabicIfNeeded(text.data(), text.size());
    ASSERT_EQ(text[0], (UNICODE_CODEPOINT)0xFE8F);
    ASSERT_EQ(text[1], (UNICODE_CODEPOINT)0xFFFC);
    ASSERT_EQ(text[2], (UNICODE_CODEPOINT)0xFE8F);
}
