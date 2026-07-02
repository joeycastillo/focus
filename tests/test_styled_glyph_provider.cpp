/*
 * Tests for StyledGlyphProvider emphasis dispatch and fallback.
 *
 * Uses BDFGlyphProvider with real font files to verify that
 * StyledGlyphProvider routes glyph/metric queries to the correct
 * sub-provider based on the emphasis parameter.
 */

#include "test_harness.hpp"
#include "StyledGlyphProvider.hpp"
#include "BDFGlyphProvider.hpp"
#include "MockGlyphProvider.hpp"
#include <memory>

using namespace focus;

// Font paths — FONTS_DIR is set by CMake to the absolute fonts/ directory
static const char* FONT_REGULAR = FONTS_DIR "lucida-bright-14.bdf";
static const char* FONT_BOLD    = FONTS_DIR "lucida-bright-24.bdf";
static const char* FONT_ITALIC  = FONTS_DIR "lucida-sans-14.bdf";

// --- supportsEmphasis ---

TEST(styled_regular_only_supports_emphasis_0) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    ASSERT_TRUE(regular->isValid());

    auto styled = std::make_shared<StyledGlyphProvider>(regular);

    ASSERT_TRUE(styled->supportsEmphasis(FontStyle::Regular));
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Italic));
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Bold));
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Italic | FontStyle::Bold));
}

TEST(styled_regular_plus_bold_supports_emphasis_2) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto bold = std::make_shared<BDFGlyphProvider>(FONT_BOLD);
    ASSERT_TRUE(regular->isValid());
    ASSERT_TRUE(bold->isValid());

    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    ASSERT_TRUE(styled->supportsEmphasis(FontStyle::Regular));
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Italic));
    ASSERT_TRUE(styled->supportsEmphasis(FontStyle::Bold));
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Italic | FontStyle::Bold));
}

// --- metricsForCodepoint dispatch ---

TEST(styled_metrics_emphasis_0_returns_regular) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto bold = std::make_shared<BDFGlyphProvider>(FONT_BOLD);

    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    GlyphMetrics styledMetrics = styled->metricsForCodepoint('A', FontStyle::Regular);
    GlyphMetrics regularMetrics = regular->metricsForCodepoint('A');

    ASSERT_EQ(styledMetrics.advance, regularMetrics.advance);
    ASSERT_EQ(styledMetrics.height, regularMetrics.height);
    ASSERT_EQ(styledMetrics.xOffset, regularMetrics.xOffset);
    ASSERT_EQ(styledMetrics.yOffset, regularMetrics.yOffset);
}

TEST(styled_metrics_emphasis_2_returns_bold) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto bold = std::make_shared<BDFGlyphProvider>(FONT_BOLD);

    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    GlyphMetrics styledMetrics = styled->metricsForCodepoint('A', FontStyle::Bold);
    GlyphMetrics boldMetrics = bold->metricsForCodepoint('A');

    ASSERT_EQ(styledMetrics.advance, boldMetrics.advance);
    ASSERT_EQ(styledMetrics.height, boldMetrics.height);
    ASSERT_EQ(styledMetrics.xOffset, boldMetrics.xOffset);
    ASSERT_EQ(styledMetrics.yOffset, boldMetrics.yOffset);
}

TEST(styled_metrics_emphasis_2_fallback_when_no_bold) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);

    // No bold provider — should fall back to regular
    auto styled = std::make_shared<StyledGlyphProvider>(regular);

    GlyphMetrics styledMetrics = styled->metricsForCodepoint('A', FontStyle::Bold);
    GlyphMetrics regularMetrics = regular->metricsForCodepoint('A');

    ASSERT_EQ(styledMetrics.advance, regularMetrics.advance);
    ASSERT_EQ(styledMetrics.height, regularMetrics.height);
    ASSERT_EQ(styledMetrics.xOffset, regularMetrics.xOffset);
    ASSERT_EQ(styledMetrics.yOffset, regularMetrics.yOffset);
}

// --- glyphForCodepoint dispatch ---

TEST(styled_glyph_emphasis_1_returns_italic) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto italic = std::make_shared<BDFGlyphProvider>(FONT_ITALIC);
    ASSERT_TRUE(italic->isValid());

    // Constructor: regular, italic (bold defaults to nullptr)
    auto styled = std::make_shared<StyledGlyphProvider>(regular, italic);

    const uint8_t* styledGlyph = styled->glyphForCodepoint('A', FontStyle::Italic);
    const uint8_t* italicGlyph = italic->glyphForCodepoint('A');

    // Both should point to the same underlying data
    ASSERT_TRUE(styledGlyph != nullptr);
    ASSERT_TRUE(italicGlyph != nullptr);
    ASSERT_EQ(styledGlyph, italicGlyph);
}

TEST(styled_glyph_emphasis_1_fallback_when_no_italic) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);

    // No italic provider — should fall back to regular
    auto styled = std::make_shared<StyledGlyphProvider>(regular);

    const uint8_t* styledGlyph = styled->glyphForCodepoint('A', FontStyle::Italic);
    const uint8_t* regularGlyph = regular->glyphForCodepoint('A');

    ASSERT_TRUE(styledGlyph != nullptr);
    ASSERT_TRUE(regularGlyph != nullptr);
    ASSERT_EQ(styledGlyph, regularGlyph);
}

// --- Font-level metadata always from regular ---

TEST(styled_glyph_row_count_from_regular) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto bold = std::make_shared<BDFGlyphProvider>(FONT_BOLD);

    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    ASSERT_EQ(styled->getGlyphRowCount(), regular->getGlyphRowCount());
}

// --- Out-of-range emphasis falls back to regular ---

TEST(styled_out_of_range_emphasis_falls_back) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto styled = std::make_shared<StyledGlyphProvider>(regular);

    // Emphasis 5 is out of range — should fall back to regular
    GlyphMetrics styledMetrics = styled->metricsForCodepoint('A', static_cast<FontStyle>(5));
    GlyphMetrics regularMetrics = regular->metricsForCodepoint('A');

    ASSERT_EQ(styledMetrics.advance, regularMetrics.advance);
    ASSERT_EQ(styledMetrics.height, regularMetrics.height);

    ASSERT_FALSE(styled->supportsEmphasis(static_cast<FontStyle>(5)));
    ASSERT_FALSE(styled->supportsEmphasis(static_cast<FontStyle>(255)));
}

// --- Bold+italic fallback chain (emphasis=3) ---

TEST(styled_emphasis_3_uses_bold_italic_provider) {
    auto regular = std::make_shared<MockGlyphProvider>(8, 14);
    auto italic = std::make_shared<MockGlyphProvider>(9, 14);
    auto bold = std::make_shared<MockGlyphProvider>(10, 14);
    auto boldItalic = std::make_shared<MockGlyphProvider>(11, 14);

    auto styled = std::make_shared<StyledGlyphProvider>(regular, italic, bold, boldItalic);

    GlyphMetrics m = styled->metricsForCodepoint('A', FontStyle::Italic | FontStyle::Bold);
    ASSERT_EQ(m.advance, 11); // from boldItalic
    ASSERT_TRUE(styled->supportsEmphasis(FontStyle::Italic | FontStyle::Bold));
}

TEST(styled_emphasis_3_falls_back_to_italic_when_no_bi) {
    auto regular = std::make_shared<MockGlyphProvider>(8, 14);
    auto italic = std::make_shared<MockGlyphProvider>(9, 14);
    auto bold = std::make_shared<MockGlyphProvider>(10, 14);

    // No boldItalic — should prefer italic over bold
    auto styled = std::make_shared<StyledGlyphProvider>(regular, italic, bold);

    GlyphMetrics m = styled->metricsForCodepoint('A', FontStyle::Italic | FontStyle::Bold);
    ASSERT_EQ(m.advance, 9); // from italic (preferred over bold)
    ASSERT_FALSE(styled->supportsEmphasis(FontStyle::Italic | FontStyle::Bold));
}

TEST(styled_emphasis_3_falls_back_to_bold_when_no_bi_no_italic) {
    auto regular = std::make_shared<MockGlyphProvider>(8, 14);
    auto bold = std::make_shared<MockGlyphProvider>(10, 14);

    // No boldItalic, no italic — should fall back to bold
    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    GlyphMetrics m = styled->metricsForCodepoint('A', FontStyle::Italic | FontStyle::Bold);
    ASSERT_EQ(m.advance, 10); // from bold
}

TEST(styled_emphasis_3_falls_back_to_regular_when_nothing) {
    auto regular = std::make_shared<MockGlyphProvider>(8, 14);

    // Only regular — emphasis=3 falls all the way back
    auto styled = std::make_shared<StyledGlyphProvider>(regular);

    GlyphMetrics m = styled->metricsForCodepoint('A', FontStyle::Italic | FontStyle::Bold);
    ASSERT_EQ(m.advance, 8); // from regular
}

// --- hasGlyph ---

TEST(mock_has_glyph_default_returns_true) {
    MockGlyphProvider mock;
    ASSERT_TRUE(mock.hasGlyph('A'));
    ASSERT_TRUE(mock.hasGlyph(0x0391)); // Greek Alpha
}

TEST(mock_has_glyph_limited_set) {
    MockGlyphProvider mock;
    mock.limitedGlyphSet = {'A', 'B', 'C'};
    ASSERT_TRUE(mock.hasGlyph('A'));
    ASSERT_FALSE(mock.hasGlyph('Z'));
    ASSERT_FALSE(mock.hasGlyph(0x0391));
}

TEST(bdf_has_glyph_ascii) {
    auto font = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    ASSERT_TRUE(font->isValid());
    ASSERT_TRUE(font->hasGlyph('A'));
    ASSERT_TRUE(font->hasGlyph(' '));
}

TEST(bdf_has_glyph_missing_codepoint) {
    auto font = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    ASSERT_TRUE(font->isValid());
    // CJK ideograph — Lucida Bright does not contain this
    ASSERT_FALSE(font->hasGlyph(0x4E00));
}

TEST(styled_has_glyph_delegates_to_regular) {
    auto regular = std::make_shared<BDFGlyphProvider>(FONT_REGULAR);
    auto bold = std::make_shared<BDFGlyphProvider>(FONT_BOLD);
    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, bold);

    ASSERT_TRUE(styled->hasGlyph('A'));
    ASSERT_FALSE(styled->hasGlyph(0x4E00));
}
