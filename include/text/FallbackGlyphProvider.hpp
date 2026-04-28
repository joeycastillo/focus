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
 * @file FallbackGlyphProvider.hpp
 * @brief A GlyphProvider that delegates to a fallback font for missing glyphs.
 *
 * FallbackGlyphProvider wraps a primary provider and a fallback provider (e.g.,
 * Unifont). For each codepoint, it checks whether the primary has a real glyph
 * via hasGlyph(). If so, all queries delegate to the primary. If not, it
 * reformats the fallback glyph into a bitmap compatible with the primary font's
 * cell dimensions, applying baseline alignment and synthetic emphasis (bold,
 * italic) as needed.
 *
 * Font-level metadata (point size, max size, offset, row count, title, validity)
 * always comes from the primary. supportsEmphasis() also mirrors the primary:
 * if the primary reports bold support, FallbackGlyphProvider takes responsibility
 * for baking synthetic bold into fallback glyphs, preventing the renderer from
 * double-applying it.
 *
 * @ingroup text
 */

#pragma once

#include "GlyphProvider.hpp"
#include <memory>
#include <vector>

/**
 * @brief GlyphProvider that falls back to a secondary font for missing glyphs.
 *
 * @par Usage
 * Wrap a primary reading font (e.g., Lucida) with a fallback (e.g., Unifont).
 * The primary's glyph coverage determines which glyphs are "owned" by the
 * primary; everything else falls through to the fallback.
 *
 * @par Emphasis handling
 * supportsEmphasis() delegates to the primary. When the primary reports support
 * for bold or italic, the FallbackGlyphProvider bakes synthetic emphasis into
 * fallback bitmaps so the renderer does not double-apply effects.
 *
 * @ingroup text
 */
class FallbackGlyphProvider : public GlyphProvider {
public:
    /**
     * @brief Create a fallback provider wrapping a primary and fallback font.
     * @param primary The primary font provider. Must not be nullptr.
     * @param fallback The fallback font provider. Must not be nullptr.
     * @param fallbackAscent Number of rows of ascent in the fallback font.
     *        Used for baseline alignment. If 0, the fallback glyph is positioned
     *        so its top aligns with (primaryAscent - fallbackHeight).
     */
    FallbackGlyphProvider(
        std::shared_ptr<GlyphProvider> primary,
        std::shared_ptr<GlyphProvider> fallback,
        uint8_t fallbackAscent = 0);

    // GlyphProvider interface — font-level metadata from primary
    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    bool isValid() const override;
    std::string getTitle() const override;

    // Per-glyph queries with fallback
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;

    bool supportsEmphasis(uint8_t emphasis) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;

private:
    void reformatFallbackGlyph(UNICODE_CODEPOINT codepoint, uint8_t emphasis) const;

    std::shared_ptr<GlyphProvider> primary;
    std::shared_ptr<GlyphProvider> fallback;
    uint8_t fallbackAscent;
    mutable std::vector<uint8_t> glyphBuffer;
};
