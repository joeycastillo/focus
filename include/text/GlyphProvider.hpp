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
 */

#pragma once

#include "Focus.hpp"
#include "utf8_decode.hpp"

/**
 * @brief Abstract base class for glyph bitmap and metric providers.
 *
 * Subclasses must implement all pure virtual methods to supply glyph data
 * for text rendering and layout. The Font class wraps a GlyphProvider and
 * provides caching and factory methods.
 */
class GlyphProvider : public std::enable_shared_from_this<GlyphProvider> {
public:
    GlyphProvider();

    /// @brief Get the font's point size (pixel height).
    virtual uint8_t getPointSize() = 0;

    /// @brief Get the maximum glyph bounding box size across all glyphs.
    virtual Size getMaxSize() = 0;

    /// @brief Get the global glyph offset (baseline adjustment).
    virtual Point getOffset() = 0;

    /**
     * @brief Get the number of byte rows per glyph bitmap.
     *
     * Each glyph bitmap is stored as rows of bytes, packed MSB-first.
     * This returns the total height of the glyph storage area (ascent + descent).
     */
    virtual uint8_t getGlyphRowCount() = 0;

    /// @brief Check whether the font was loaded successfully.
    virtual bool isValid() const = 0;

    /**
     * @brief Get the 1bpp bitmap data for a Unicode codepoint.
     *
     * Returns a pointer to the glyph's bitmap in MSB-first, row-major format.
     * The caller should use metricsForCodepoint() to determine the bitmap's
     * dimensions and positioning.
     *
     * @param codepoint The Unicode codepoint to look up.
     * @param font Optional font name (unused by most providers).
     * @return Pointer to the glyph bitmap, or nullptr if not found.
     */
    virtual uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) = 0;

    /**
     * @brief Get the metrics (bounding box) for a Unicode codepoint.
     *
     * Returns a Rect where:
     * - origin.x = horizontal offset from the cursor position
     * - origin.y = vertical offset from the baseline (positive = up)
     * - size.width = advance width (how far to move the cursor)
     * - size.height = bitmap height
     *
     * @param codepoint The Unicode codepoint to look up.
     * @param font Optional font name (unused by most providers).
     * @return Bounding box and advance metrics for the glyph.
     */
    virtual Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) = 0;
};
