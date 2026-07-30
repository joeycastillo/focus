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
 * @file PackedFontGlyphProvider.hpp
 * @brief GlyphProvider implementation that loads fonts from BDP (Bitmap Distribution Packed) files.
 *
 * PackedFontGlyphProvider reads compact binary font files produced by bdf_to_bdp.py.
 * BDP is a binary encoding of BDF font data that eliminates text parsing overhead
 * and reduces file size by ~4x compared to BDF.
 */

#pragma once

#include "GlyphProvider.hpp"
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace focus {

/// @brief Parsed data for a single BDP glyph.
struct BDPGlyph {
    uint8_t width;       ///< Tight bitmap width in pixels.
    uint8_t height;      ///< Tight bitmap height in pixels.
    int8_t xOffset;      ///< Horizontal offset from cursor position.
    int8_t yOffset;      ///< Vertical offset from baseline (positive = above).
    uint8_t advance;     ///< Advance width (cursor movement).
    std::vector<uint8_t> bitmap;  ///< Bitmap in display format (padded to full row count).
};

/// @brief GlyphProvider that loads and serves glyphs from BDP font files.
///
/// @par Memory
/// All glyphs are held in memory for the lifetime of the provider. This is
/// efficient for small application-specific fonts (100-500 glyphs) but can
/// use significant memory for full-Unicode fonts (10,000+ glyphs). For large
/// character sets, prefer UnifontGlyphProvider, which streams glyphs on demand.
///
/// @todo Lazy loading: load glyph bitmaps on demand rather than all at
/// construction. The const query interface already supports this via mutable
/// caching (see UnifontGlyphProvider for the pattern).
/// @ingroup text
class PackedFontGlyphProvider : public GlyphProvider {
public:
    PackedFontGlyphProvider(const std::string& bdpFilePath);

    /**
     * Build a provider from an in-memory BDP blob (same bytes as a .bdp file).
     * Non-owning: glyphs are copied out, so `data` need only outlive this call.
     * @return the provider, or nullptr if the blob is not valid BDP.
     */
    static std::shared_ptr<PackedFontGlyphProvider> fromMemory(const uint8_t* data, size_t size);

    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;

    bool isValid() const override { return valid; }
    std::string getTitle() const override { return title; }
    size_t getGlyphCount() const { return glyphs.size(); }

    /// Read just the title from a BDP file without loading glyphs.
    /// Returns an empty string if the file has no title or cannot be read.
    static std::string readTitle(const std::string& path);

private:
    PackedFontGlyphProvider() = default;
    bool loadBDPFile(const std::string& path);
    bool loadBDP(const uint8_t* data, size_t size);
    void convertGlyphToDisplayFormat(BDPGlyph& glyph);

    std::unordered_map<uint32_t, BDPGlyph> glyphs;
    std::string title;

    uint8_t pixelSize = 0;
    uint8_t fontAscent = 0;
    uint8_t fontDescent = 0;
    Size maxSize = {0, 0};
    uint32_t defaultChar = 0;
    bool valid = false;
};

}  // namespace focus
