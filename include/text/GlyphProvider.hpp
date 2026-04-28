/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
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
 * @file GlyphProvider.hpp
 * @brief Abstract interface for providing glyph bitmaps and metrics.
 *
 * GlyphProvider defines the contract for font backends that supply glyph
 * data to the text rendering system. Implementations include BasicGlyphProvider
 * (a built-in fixed-width fallback) and BDFGlyphProvider (BDF font files).
 *
 * This interface is consumed by the text rendering stack: TextLayout uses it
 * for measurement and word-wrapping, and CanvasView uses it for glyph
 * rendering. Application-level code (e.g. pagination engines) also passes
 * it through to TextLayout. All layers share the same GlyphProvider instance
 * to ensure that measurement and rendering always use identical metrics.
 */

#pragma once

#include "Focus.hpp"
#include "utf8_decode.hpp"
#include <string>
#include <vector>

/**
 * @brief Metrics for a single glyph: advance, bitmap dimensions, and positioning.
 *
 * Returned by GlyphProvider::metricsForCodepoint(). All widths are in pixels.
 * @ingroup text
 */
struct GlyphMetrics {
    uint8_t advance;      ///< Cursor movement after rendering (DWIDTH).
    uint8_t bitmapWidth;  ///< Actual bitmap width in pixels (BBX width). May exceed advance for overhanging glyphs.
    uint8_t height;       ///< Bitmap height in pixels (BBX height).
    int8_t xOffset;       ///< Horizontal offset from cursor position (BBX x).
    int8_t yOffset;       ///< Vertical offset from baseline, positive = above (BBX y).
};

/**
 * @brief Abstract base class for glyph bitmap and metric providers.
 *
 * Subclasses must implement all pure virtual methods to supply glyph data
 * for text rendering and layout. The Font class wraps a GlyphProvider and
 * provides caching and factory methods.
 * @ingroup text
 */
class GlyphProvider {
public:
    GlyphProvider();
    virtual ~GlyphProvider() = default;

    /**
     * @brief Get a pre-populated cache of metrics for ASCII codepoints 0x20..0x7F.
     *
     * Returns a pointer to a 96-element GlyphMetrics array. Index with (codepoint - 0x20).
     * Populated lazily on first call from metricsForCodepoint().
     * Used by TextLayout to skip virtual dispatch and hash lookups in the hot loop.
     */
    const GlyphMetrics* getAsciiMetricsCache() const;

    /// @brief Get the font's point size (pixel height).
    virtual uint8_t getPointSize() const = 0;

    /// @brief Get the maximum glyph bounding box size across all glyphs.
    virtual Size getMaxSize() const = 0;

    /// @brief Get the global glyph offset (baseline adjustment).
    virtual Point getOffset() const = 0;

    /**
     * @brief Get the number of byte rows per glyph bitmap.
     *
     * Each glyph bitmap is stored as rows of bytes, packed MSB-first.
     * This returns the total height of the glyph storage area (ascent + descent).
     */
    virtual uint8_t getGlyphRowCount() const = 0;

    /// @brief Check whether the font was loaded successfully.
    virtual bool isValid() const = 0;

    /// @brief Get the human-readable title embedded in the font file (e.g., "Times 12pt").
    /// Returns an empty string if no title is available.
    virtual std::string getTitle() const { return ""; }

    /**
     * @brief Get the 1bpp bitmap data for a Unicode codepoint.
     *
     * Returns a pointer to the glyph's bitmap in MSB-first, row-major format.
     * The caller should use metricsForCodepoint() to determine the bitmap's
     * dimensions and positioning.
     *
     * @param codepoint The Unicode codepoint to look up.
     * @return Pointer to the glyph bitmap, or nullptr if not found.
     *         The pointer is valid only until the next call to
     *         glyphForCodepoint() on the same provider.
     */
    virtual const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const = 0;

    /**
     * @brief Get the metrics for a Unicode codepoint.
     *
     * @param codepoint The Unicode codepoint to look up.
     * @param emphasis Style variant: 0=regular, 1=italic, 2=bold, 3=bold+italic.
     * @return GlyphMetrics with advance, bitmap dimensions, and positioning.
     */
    virtual GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const = 0;

    /// Query whether this provider has a real font for the given emphasis level.
    /// @param emphasis 0=regular, 1=italic, 2=bold, 3=bold+italic
    /// @return true if real variant glyphs are available (not synthetic).
    virtual bool supportsEmphasis(uint8_t emphasis) const { return emphasis == 0; }

    /// Query whether this provider has a real glyph for the given codepoint.
    /// Returns true if the provider can render this codepoint with its own font
    /// data (not a fallback/replacement glyph). Used by FallbackGlyphProvider
    /// to decide when to delegate to a fallback font.
    /// @param codepoint The Unicode codepoint to check.
    /// @return true if a real glyph exists. Default returns true (conservative).
    virtual bool hasGlyph(UNICODE_CODEPOINT codepoint) const { return true; }

protected:
    /**
     * @brief Convert a glyph bitmap to the padded display format.
     *
     * Repositions the glyph within a full-height (fontAscent + fontDescent)
     * row buffer, sized to the advance width. Shared by BDF and PackedFont
     * providers which use identical conversion logic.
     *
     * @param bitmap The source bitmap data (modified in place with the converted result).
     * @param width Tight bitmap width in pixels.
     * @param height Tight bitmap height in pixels.
     * @param yOffset Vertical offset from baseline (positive = above).
     * @param advance Advance width (cursor movement).
     * @param fontAscent Pixels above the baseline for this font.
     * @param fontDescent Pixels below the baseline for this font.
     */
    static void convertBitmapToDisplayFormat(
        std::vector<uint8_t>& bitmap,
        uint8_t width, uint8_t height, int8_t yOffset, uint8_t advance,
        uint8_t fontAscent, uint8_t fontDescent);

private:
    mutable GlyphMetrics asciiMetricsCache[96]; ///< Cached metrics for codepoints 0x20..0x7F.
    mutable bool asciiCachePopulated = false;
};
