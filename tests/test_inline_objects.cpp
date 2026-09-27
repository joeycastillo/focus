/*
 * Inline objects: U+FFFC takes a caller-supplied width, is broken around
 * like an atomic inline, and reports where it lands.
 */

#include "test_harness.hpp"
#include "MockGlyphProvider.hpp"
#include "TextLayout.hpp"
#include "InlineObjectProvider.hpp"
#include "Utf8.hpp"
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
