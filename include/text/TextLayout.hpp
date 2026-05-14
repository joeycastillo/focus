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
 * @file TextLayout.hpp
 * @brief Shared text measurement and word-wrapping primitives.
 *
 * TextLayout provides static methods for measuring text width, calculating
 * line heights, and determining word-wrap break points. It is the measurement
 * layer used by Focus's text rendering and by application-level pagination
 * engines. A framing engine calls measureLineWrap in a loop to determine
 * where each line breaks, then stacks the results vertically to fill a page.
 *
 * These primitives are also available directly for simpler use cases that
 * don't need full page layout (e.g. measuring a label's width, or checking
 * whether a string fits in a given area).
 */

#pragma once

#include "Focus.hpp"
#include "GlyphProvider.hpp"
#include "Hyphenator.hpp"
#include "UnicodeTraits.hpp"
#include <cstdint>
#include <cstddef>

/// Result of a word wrap measurement.
struct WordWrapResult {
    /// Number of codepoints that fit on this line, or negative if no wrap was
    /// needed (all remaining codepoints were consumed without exceeding the
    /// layout width). A negative value means the input ended mid-line — either
    /// the text genuinely ended, or the buffer ran out. A framing engine
    /// uses the sign to distinguish complete lines from partial lines at
    /// chunk boundaries.
    int32_t codepointsConsumed;

    bool wrapped;                ///< True if line was wrapped (false if ended at newline or end of text).
    bool isParagraphBreak;       ///< True if line ended with a newline character.
    bool needsHyphen = false;    ///< True if a trailing hyphen should be rendered after this line.
    int16_t endCursorX;          ///< Horizontal cursor position after processing.
};

/// Low-level text measurement shared across the Focus text subsystem.
/// Used by Display, CanvasView, and application-level pagination engines.
/// All word-wrapping flows through measureLineWrap, ensuring that measurement
/// during pagination and measurement during rendering always agree.
/// @ingroup text
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
    /// @param initialCursorX Starting X position (for continuing partial lines across chunks)
    /// @param initialEmphasis Starting emphasis depth (0-3) for style-aware measurement
    /// @return WordWrapResult containing wrap position and metadata
    static WordWrapResult measureLineWrap(
        UNICODE_CODEPOINT* codepoints,
        size_t len,
        int16_t layoutWidth,
        uint8_t textSize,
        const GlyphProvider* glyphProvider,
        int16_t initialCursorX = 0,
        uint8_t initialEmphasis = 0,
        const Hyphenator* hyphenator = nullptr
    );

    /// Calculate line height for wrapped lines
    /// @param glyphProvider Provider for font metrics
    /// @param textSize Text scaling factor
    /// @param lineSpacing Additional spacing between lines
    static int16_t getLineHeight(const GlyphProvider* glyphProvider, uint8_t textSize, int16_t lineSpacing);

    /// Calculate paragraph height (line height + extra paragraph spacing)
    /// @param glyphProvider Provider for font metrics
    /// @param textSize Text scaling factor
    /// @param paragraphSpacing Total spacing after paragraph break
    static int16_t getParagraphHeight(const GlyphProvider* glyphProvider, uint8_t textSize, int16_t paragraphSpacing);

    /// Calculate default line spacing (constant 2 pixels)
    static int16_t calculateLineSpacing(const GlyphProvider* glyphProvider);

    /// Calculate default paragraph spacing based on glyph row count
    static int16_t calculateParagraphSpacing(const GlyphProvider* glyphProvider);

    /// Measure the width of a UTF-8 text string in pixels.
    /// @param utf8String The UTF-8 encoded string to measure
    /// @param textSize Text scaling factor (1 = normal)
    /// @param glyphProvider Provider for glyph metrics
    /// @return Width of the text in pixels
    static int16_t measureTextWidth(const char* utf8String, uint8_t textSize, const GlyphProvider* glyphProvider);

    /// Measure the total height of a UTF-8 text string when word-wrapped
    /// to a given layout width.
    /// @param utf8String The UTF-8 encoded string to measure
    /// @param layoutWidth Available width in pixels for word wrapping
    /// @param textSize Text scaling factor (1 = normal)
    /// @param glyphProvider Provider for glyph metrics
    /// @return Total height in pixels, including line and paragraph spacing
    static int16_t measureTextHeight(const char* utf8String, int16_t layoutWidth, uint8_t textSize, const GlyphProvider* glyphProvider);

};