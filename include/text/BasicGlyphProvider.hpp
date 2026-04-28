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
 * @file BasicGlyphProvider.hpp
 * @brief Built-in fixed-width glyph provider used as a fallback font.
 *
 * BasicGlyphProvider supplies a minimal 5x8 pixel fixed-width ASCII font
 * that is compiled into the binary. It covers printable ASCII (0x20-0x7E)
 * and is used as a fallback when no external font files are available.
 */

#pragma once

#include "Focus.hpp"
#include "GlyphProvider.hpp"

/**
 * @brief A minimal built-in 5x8 fixed-width glyph provider.
 *
 * Always reports isValid() as true since the font data is compiled in.
 * Used as a fallback when BDF font files cannot be loaded.
 * @ingroup text
 */
class BasicGlyphProvider : public GlyphProvider {
public:
    BasicGlyphProvider();
    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    /// @brief Always returns true (built-in font data is always available).
    bool isValid() const override { return true; }
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;
};
