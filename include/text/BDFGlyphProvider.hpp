/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file BDFGlyphProvider.hpp
 * @brief GlyphProvider implementation that loads fonts from BDF (Bitmap Distribution Format) files.
 *
 * BDFGlyphProvider parses standard BDF font files and provides glyph bitmaps
 * and metrics for text rendering. BDF is a widely supported bitmap font format
 * that can represent variable-width glyphs with per-glyph bounding boxes.
 *
 * Glyph bitmaps are converted from the BDF hex encoding to the Focus display
 * format (MSB-first, row-major, padded to the font's full row count) during loading.
 */

#pragma once

#include "GlyphProvider.hpp"
#include "focus_config.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace focus {

#if FOCUS_HAS_FILESYSTEM

/**
 * @brief Parsed data for a single BDF glyph.
 *
 * Contains the glyph's bounding box, advance width, and the converted
 * bitmap ready for display rendering.
 */
struct BDFGlyph {
    uint8_t width;       ///< BBX width (bitmap width in pixels).
    uint8_t height;      ///< BBX height (original height before padding).
    int8_t xOffset;      ///< BBX horizontal offset from the cursor position.
    int8_t yOffset;      ///< BBX vertical offset from the baseline (positive = above).
    uint8_t advance;     ///< DWIDTH advance width (how far to move the cursor).
    std::vector<uint8_t> bitmap;  ///< Bitmap converted to display format (padded to full row count).
};

/**
 * @brief GlyphProvider that loads and serves glyphs from BDF font files.
 *
 * On construction, the entire BDF file is parsed and all glyphs are converted
 * to the display bitmap format. Check isValid() after construction to verify
 * the file was loaded successfully.
 *
 * @par Memory
 * All glyphs are held in memory for the lifetime of the provider. This is
 * efficient for small application-specific fonts (100-500 glyphs) but can
 * use significant memory for full-Unicode fonts (10,000+ glyphs). For large
 * character sets, prefer UnifontGlyphProvider, which streams glyphs on demand.
 *
 * @todo Lazy loading: load glyph bitmaps on demand rather than all at
 * construction. The const query interface already supports this via mutable
 * caching (see UnifontGlyphProvider for the pattern).
 * @ingroup text
 */
class BDFGlyphProvider : public GlyphProvider {
public:
    /**
     * @brief Load a BDF font file and parse all glyphs.
     * @param bdfFilePath Path to the BDF font file (e.g., "/system/fonts/lucida-bright-14.bdf").
     */
    BDFGlyphProvider(const std::string& bdfFilePath);

    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;

    /// @brief Check if the BDF file was parsed successfully.
    bool isValid() const override { return valid; }

    /// @brief Get the total number of glyphs loaded from the BDF file.
    size_t getGlyphCount() const { return glyphs.size(); }

private:
    /// @brief Parse a BDF file, populating the glyphs map and font metrics.
    bool parseBDFFile(const std::string& path);

    /// @brief Convert a glyph's bitmap from BDF hex format to padded display format.
    void convertGlyphToDisplayFormat(BDFGlyph& glyph);

    static uint8_t hexCharToNibble(char c);
    static uint8_t hexToByte(const char* hex);

    std::unordered_map<uint32_t, BDFGlyph> glyphs; ///< Codepoint-to-glyph lookup table.

    uint8_t pixelSize = 0;      ///< PIXEL_SIZE from the BDF file.
    uint8_t fontAscent = 0;     ///< FONT_ASCENT: pixels above the baseline.
    uint8_t fontDescent = 0;    ///< FONT_DESCENT: pixels below the baseline.
    Size maxSize = {0, 0};      ///< Maximum bounding box across all glyphs.
    uint32_t defaultChar = 0;   ///< DEFAULT_CHAR codepoint for missing glyphs.
    bool valid = false;         ///< Whether the BDF file was parsed successfully.
};

#endif  // FOCUS_HAS_FILESYSTEM

}  // namespace focus
