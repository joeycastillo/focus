/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#pragma once

#include "Focus.hpp"
#include "GlyphProvider.hpp"
#include "unicodetraits.hpp"
#include <cstdint>
#include <cstddef>

/// Result of a word wrap measurement
struct WordWrapResult {
    int32_t codepointsConsumed;  ///< Number of codepoints on this line (-1 if no wrap needed)
    size_t bytesConsumed;        ///< Number of UTF-8 bytes consumed
    bool wrapped;                ///< True if line was wrapped (false if ended at newline or end of text)
    bool isParagraphBreak;       ///< True if line ended with a newline character
};

/// Shared text layout engine for consistent text measurement and pagination.
/// Used by both Display (for rendering) and TextPaginator (for page calculation).
class TextLayout {
public:
    /// Calculate the UTF-8 byte count for a Unicode codepoint
    static size_t bytesForCodepoint(UNICODE_CODEPOINT cp);

    /// Measure where to wrap a line of text.
    /// @param codepoints Array of Unicode codepoints to measure
    /// @param len Number of codepoints in the array
    /// @param layoutWidth Width of the layout area in pixels
    /// @param textSize Text scaling factor (1 = normal)
    /// @param glyphProvider Provider for glyph metrics
    /// @return WordWrapResult containing wrap position and metadata
    static WordWrapResult measureLineWrap(
        UNICODE_CODEPOINT* codepoints,
        size_t len,
        int16_t layoutWidth,
        uint8_t textSize,
        GlyphProvider* glyphProvider
    );

    /// Calculate line height for wrapped lines
    /// @param glyphProvider Provider for font metrics
    /// @param textSize Text scaling factor
    /// @param lineSpacing Additional spacing between lines
    static int16_t getLineHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t lineSpacing);

    /// Calculate paragraph height (line height + extra paragraph spacing)
    /// @param glyphProvider Provider for font metrics
    /// @param textSize Text scaling factor
    /// @param paragraphSpacing Total spacing after paragraph break
    static int16_t getParagraphHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t paragraphSpacing);

    /// Calculate default line spacing based on glyph provider offset
    static int16_t calculateLineSpacing(GlyphProvider* glyphProvider);

    /// Calculate default paragraph spacing based on line spacing
    static int16_t calculateParagraphSpacing(int16_t lineSpacing);
};