/*
 * Tests for bitmap manipulation utilities and FallbackGlyphProvider.
 */

#include "test_harness.hpp"
#include "GlyphProvider.hpp"
#include <cstring>
#include <vector>

using namespace focus;

// --- applyBoldToBitmap ---

// Helper: expose the protected static via a test subclass
class GlyphProviderTestAccess : public GlyphProvider {
public:
    using GlyphProvider::applyBoldToBitmap;
    using GlyphProvider::applyShearToBitmap;

    // Minimal stubs to satisfy pure virtuals — not used in these tests
    uint8_t getPointSize() const override { return 0; }
    Size getMaxSize() const override { return {0, 0}; }
    Point getOffset() const override { return {0, 0}; }
    uint8_t getGlyphRowCount() const override { return 0; }
    bool isValid() const override { return false; }
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT, FontStyle) const override { return nullptr; }
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT, FontStyle) const override { return {}; }
    bool hasGlyph(UNICODE_CODEPOINT) const override { return true; }
};

TEST(apply_bold_single_byte_row) {
    // A single vertical line at bit 4 (MSB-first): 0b00010000 = 0x10
    // After bold: bit 4 + bit 3 set: 0b00011000 = 0x18
    uint8_t bitmap[] = {0x10};
    GlyphProviderTestAccess::applyBoldToBitmap(bitmap, 1, 1);
    ASSERT_EQ(bitmap[0], 0x18);
}

TEST(apply_bold_preserves_existing_bits) {
    // 0b10100000 = 0xA0 -> bold: 0b11110000 = 0xF0
    uint8_t bitmap[] = {0xA0};
    GlyphProviderTestAccess::applyBoldToBitmap(bitmap, 1, 1);
    ASSERT_EQ(bitmap[0], 0xF0);
}

TEST(apply_bold_multi_byte_row_carry) {
    // Two bytes per row: [0x01, 0x00] — rightmost bit of byte 0
    // Bold shifts right: bit 0 of byte 0 carries to bit 7 of byte 1
    // Result: [0x01, 0x80]
    uint8_t bitmap[] = {0x01, 0x00};
    GlyphProviderTestAccess::applyBoldToBitmap(bitmap, 2, 1);
    ASSERT_EQ(bitmap[0], 0x01);
    ASSERT_EQ(bitmap[1], 0x80);
}

TEST(apply_bold_multiple_rows) {
    // 2 rows, 1 byte each
    uint8_t bitmap[] = {0x40, 0x20};
    GlyphProviderTestAccess::applyBoldToBitmap(bitmap, 1, 2);
    ASSERT_EQ(bitmap[0], 0x60); // 0b01000000 -> 0b01100000
    ASSERT_EQ(bitmap[1], 0x30); // 0b00100000 -> 0b00110000
}

// --- applyShearToBitmap ---

TEST(apply_shear_shifts_top_rows_right) {
    // 4 rows, 2 bytes each (to accommodate shift), shear=2
    // Shear formula: shift = shear * (rowCount - 1 - row) / (rowCount - 1)
    // Row 0: 2 * 3 / 3 = 2
    // Row 1: 2 * 2 / 3 = 1
    // Row 2: 2 * 1 / 3 = 0
    // Row 3: 2 * 0 / 3 = 0
    uint8_t bitmap[8] = {0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00};
    GlyphProviderTestAccess::applyShearToBitmap(bitmap, 2, 4, 2);
    ASSERT_EQ(bitmap[0], 0x20); // row 0: shifted right 2
    ASSERT_EQ(bitmap[2], 0x40); // row 1: shifted right 1
    ASSERT_EQ(bitmap[4], 0x80); // row 2: no shift
    ASSERT_EQ(bitmap[6], 0x80); // row 3: no shift
}

// --- FallbackGlyphProvider tests ---

#include "FallbackGlyphProvider.hpp"
#include "StyledGlyphProvider.hpp"
#include "MockGlyphProvider.hpp"

// --- FallbackGlyphProvider: basic delegation ---

TEST(fallback_returns_primary_glyph_when_present) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A', 'B', 'C'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    const uint8_t* glyph = provider->glyphForCodepoint('A');
    ASSERT_EQ(glyph, primary->glyphForCodepoint('A'));
}

TEST(fallback_returns_fallback_glyph_when_missing) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A', 'B', 'C'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    const uint8_t* glyph = provider->glyphForCodepoint(0x0391);
    ASSERT_TRUE(glyph != nullptr);
    ASSERT_NE(glyph, primary->glyphForCodepoint(0x0391));
}

TEST(fallback_metrics_from_primary_when_present) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    GlyphMetrics m = provider->metricsForCodepoint('A');
    ASSERT_EQ(m.advance, 10);
}

TEST(fallback_metrics_from_fallback_when_missing) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    GlyphMetrics m = provider->metricsForCodepoint(0x0391);
    ASSERT_EQ(m.advance, 8);
}

TEST(fallback_has_glyph_delegates_to_primary) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    ASSERT_TRUE(provider->hasGlyph('A'));
    ASSERT_FALSE(provider->hasGlyph(0x0391));
}

TEST(fallback_metadata_from_primary) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    ASSERT_EQ(provider->getGlyphRowCount(), 23);
    ASSERT_EQ(provider->getPointSize(), 23);
    ASSERT_TRUE(provider->isValid());
}

// --- FallbackGlyphProvider: emphasis handling ---

TEST(fallback_supports_emphasis_mirrors_primary) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    // MockGlyphProvider doesn't override supportsEmphasis, so default = only 0
    ASSERT_TRUE(provider->supportsEmphasis(FontStyle::Regular));
    ASSERT_FALSE(provider->supportsEmphasis(FontStyle::Italic));
    ASSERT_FALSE(provider->supportsEmphasis(FontStyle::Bold));
}

TEST(fallback_emphasis_advance_unchanged_when_primary_lacks_bold) {
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    // Primary doesn't support emphasis 2, so no bold pixel added to advance
    GlyphMetrics m = provider->metricsForCodepoint(0x0391, FontStyle::Bold);
    ASSERT_EQ(m.advance, 8); // unchanged fallback advance
}

TEST(fallback_height_max_of_primary_and_fallback) {
    // When fallback is taller than primary (Unifont32 in Lucida 16pt)
    auto primary = std::make_shared<MockGlyphProvider>(10, 30);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(16, 32);

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    GlyphMetrics m = provider->metricsForCodepoint(0x0391);
    ASSERT_EQ(m.height, 32); // max(30, 32)
}

TEST(fallback_height_uses_primary_when_larger) {
    // When primary is taller than fallback (Unifont16 in Lucida 12pt)
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->limitedGlyphSet = {'A'};
    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback);

    GlyphMetrics m = provider->metricsForCodepoint(0x0391);
    ASSERT_EQ(m.height, 23); // max(23, 16)
}

// --- Bitmap reformatting: baseline alignment ---

TEST(fallback_bitmap_baseline_alignment) {
    // Primary: 23 rows, ascent=18 (getOffset returns {0, -18})
    // Fallback: 16 rows, fallbackAscent=14
    // startRow = primaryAscent - fallbackAscent = 18 - 14 = 4
    // So the fallback bitmap should be placed starting at row 4 of the output.
    auto primary = std::make_shared<MockGlyphProvider>(10, 23);
    primary->yOffset = 18;
    primary->limitedGlyphSet = {'A'};

    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    // Set a recognizable pattern in the fallback bitmap: 0xFF in row 0
    // 8px wide = 1 byte per row
    fallback->bitmap[0] = 0xFF;

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback, 14);

    // Request a fallback glyph (codepoint not in primary)
    const uint8_t* glyph = provider->glyphForCodepoint(0x0391);
    ASSERT_TRUE(glyph != nullptr);

    // Output has max(23, 16) = 23 rows, 1 byte per row (8px wide)
    // Row 4 should have 0xFF (the fallback's row 0)
    // Rows 0-3 should be empty (baseline alignment padding)
    ASSERT_EQ(glyph[0], 0x00); // row 0: empty
    ASSERT_EQ(glyph[1], 0x00); // row 1: empty
    ASSERT_EQ(glyph[2], 0x00); // row 2: empty
    ASSERT_EQ(glyph[3], 0x00); // row 3: empty
    ASSERT_EQ(glyph[4], 0xFF); // row 4: fallback row 0
}

TEST(fallback_yoffset_when_fallback_taller) {
    // Primary: 30 rows, ascent=24
    // Fallback: 32 rows, fallbackAscent=28
    // fallbackAscent(28) > primaryAscent(24), so yOffset = 24 - 28 = -4
    auto primary = std::make_shared<MockGlyphProvider>(10, 30);
    primary->yOffset = 24;
    primary->limitedGlyphSet = {'A'};

    auto fallback = std::make_shared<MockGlyphProvider>(16, 32);

    auto provider = std::make_shared<FallbackGlyphProvider>(primary, fallback, 28);

    GlyphMetrics m = provider->metricsForCodepoint(0x0391);
    ASSERT_EQ(m.yOffset, -4);
    ASSERT_EQ(m.height, 32); // max(30, 32)
}

// --- Bitmap reformatting: synthetic emphasis on fallback glyphs ---

TEST(fallback_bold_on_fallback_glyph) {
    // Primary supports bold (via StyledGlyphProvider wrapping)
    auto regular = std::make_shared<MockGlyphProvider>(10, 23);
    regular->limitedGlyphSet = {'A'};
    auto boldFont = std::make_shared<MockGlyphProvider>(11, 23);
    boldFont->limitedGlyphSet = {'A'};

    auto styled = std::make_shared<StyledGlyphProvider>(regular, nullptr, boldFont);

    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    // Set a single pixel at bit 4 (0x10) in row 0
    fallback->bitmap[0] = 0x10;

    auto provider = std::make_shared<FallbackGlyphProvider>(styled, fallback, 14);

    // Primary supports bold
    ASSERT_TRUE(provider->supportsEmphasis(FontStyle::Bold));

    // Metrics: bold adds 1 to advance and bitmapWidth
    GlyphMetrics m = provider->metricsForCodepoint(0x0391, FontStyle::Bold);
    ASSERT_EQ(m.advance, 9);       // fallback advance(8) + 1 for bold
    ASSERT_EQ(m.bitmapWidth, 9);   // fallback bitmapWidth(8) + 1 for bold

    // Glyph: bold should OR with right-shifted copy
    // 0x10 = 0b00010000 → bold: 0b00011000 = 0x18
    const uint8_t* glyph = provider->glyphForCodepoint(0x0391, FontStyle::Bold);
    ASSERT_TRUE(glyph != nullptr);

    // Find the row where the fallback data lands (depends on baseline alignment)
    // primary ascent = 0 (MockGlyphProvider default yOffset=0 for regular in styled)
    // With yOffset=0 on regular, primaryAscent=0, startRow = 0 - 14 = -14, clamped to 0
    // So fallback bitmap starts at row 0.
    // Output bytesPerRow = (8+1+7)/8 = 2 bytes per row
    // Row 0 byte 0 should have bold applied: 0x18
    ASSERT_EQ(glyph[0], 0x18);
}

TEST(fallback_italic_on_fallback_glyph) {
    // Primary supports italic (via StyledGlyphProvider wrapping)
    auto regular = std::make_shared<MockGlyphProvider>(10, 16);
    regular->limitedGlyphSet = {'A'};
    auto italicFont = std::make_shared<MockGlyphProvider>(10, 16);
    italicFont->limitedGlyphSet = {'A'};

    auto styled = std::make_shared<StyledGlyphProvider>(regular, italicFont);

    auto fallback = std::make_shared<MockGlyphProvider>(8, 16);
    // Set pixel at MSB (0x80) in every row
    for (int i = 0; i < 16; i++) fallback->bitmap[i] = 0x80;

    auto provider = std::make_shared<FallbackGlyphProvider>(styled, fallback, 14);

    ASSERT_TRUE(provider->supportsEmphasis(FontStyle::Italic));

    // shearPixels = outputRowCount / 4 = 16 / 4 = 4
    GlyphMetrics m = provider->metricsForCodepoint(0x0391, FontStyle::Italic);
    ASSERT_EQ(m.bitmapWidth, 12); // 8 + 4 shear

    const uint8_t* glyph = provider->glyphForCodepoint(0x0391, FontStyle::Italic);
    ASSERT_TRUE(glyph != nullptr);

    // outputBytesPerRow = (12+7)/8 = 2
    // Row 0 shift = 4 * (16-1-0) / (16-1) = 4 * 15/15 = 4
    // 0x80 >> 4 = 0x08
    ASSERT_EQ(glyph[0], 0x08);

    // Bottom row (row 15) shift = 4 * 0 / 15 = 0 (no shift)
    // 0x80 stays at 0x80
    ASSERT_EQ(glyph[15 * 2], 0x80);
}

// --- Font::setFallbackResolver integration ---

#include "Font.hpp"
#include "BDFGlyphProvider.hpp"

TEST(font_family_with_resolver_wraps_with_fallback) {
    // Set up font search path for test BDF files
    Font::setFontSearchPath(FONTS_DIR);

    // Create a mock fallback provider
    auto fallbackProvider = std::make_shared<MockGlyphProvider>(8, 16);

    // Register resolver
    Font::setFallbackResolver([fallbackProvider](const GlyphProvider* primary) -> Font::FallbackFont {
        return {fallbackProvider, 14};
    });

    // Load a font family — should be wrapped with FallbackGlyphProvider
    auto font = Font::familyWithName("lucida-bright-14");
    ASSERT_TRUE(font != nullptr);
    ASSERT_TRUE(font->isValid());

    // The font should have Lucida's metrics (from primary)
    auto provider = font->getGlyphProvider();
    ASSERT_TRUE(provider->hasGlyph('A'));  // Lucida has ASCII

    // Clean up: clear resolver and cache so other tests aren't affected
    Font::setFallbackResolver(nullptr);
    Font::clearCache();
}

TEST(font_family_without_resolver_unchanged) {
    Font::setFontSearchPath(FONTS_DIR);
    Font::setFallbackResolver(nullptr);

    auto font = Font::familyWithName("lucida-bright-14");
    ASSERT_TRUE(font != nullptr);
    ASSERT_TRUE(font->isValid());

    // Without resolver, hasGlyph for CJK should still return based on
    // the raw font (no fallback wrapping)
    auto provider = font->getGlyphProvider();
    ASSERT_FALSE(provider->hasGlyph(0x4E00));

    Font::clearCache();
}

// --- BasicGlyphProvider::hasGlyph ---

#include "BasicGlyphProvider.hpp"

TEST(basic_has_glyph_ascii_printable) {
    BasicGlyphProvider basic;
    ASSERT_TRUE(basic.hasGlyph('A'));
    ASSERT_TRUE(basic.hasGlyph(' '));  // 0x20
    ASSERT_TRUE(basic.hasGlyph('~'));  // 0x7E
}

TEST(basic_has_glyph_outside_ascii) {
    BasicGlyphProvider basic;
    ASSERT_FALSE(basic.hasGlyph(0x0391));  // Greek Alpha
    ASSERT_FALSE(basic.hasGlyph(0x1F));    // Below printable range
    ASSERT_FALSE(basic.hasGlyph(0x7F));    // DEL
    ASSERT_FALSE(basic.hasGlyph(0x4E00));  // CJK
}

// --- System font default (built-in BasicGlyphProvider floor) ---

TEST(system_font_defaults_to_basic) {
    Font::setSystemFont(nullptr);  // reset whatever earlier tests installed

    auto sys = Font::systemFont();
    ASSERT_TRUE(sys != nullptr);
    ASSERT_TRUE(sys->isValid());
    ASSERT_TRUE(sys->getGlyphProvider()->hasGlyph('A'));
    ASSERT_FALSE(sys->getGlyphProvider()->hasGlyph(0x4E00));

    // Repeated calls return the same installed default, not a fresh font
    ASSERT_TRUE(Font::systemFont() == sys);

    // Unset large/small slots chain to the same default
    ASSERT_TRUE(Font::systemLargeFont() == sys);
    ASSERT_TRUE(Font::systemSmallFont() == sys);
}

TEST(system_font_set_overrides_null_resets) {
    auto custom = Font::withProvider(std::make_shared<MockGlyphProvider>(8, 16));
    ASSERT_TRUE(custom != nullptr);

    Font::setSystemFont(custom);
    ASSERT_TRUE(Font::systemFont() == custom);

    Font::setSystemFont(nullptr);
    auto sys = Font::systemFont();
    ASSERT_TRUE(sys != nullptr);
    ASSERT_TRUE(sys != custom);
    ASSERT_TRUE(sys->isValid());
}
