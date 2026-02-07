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

#include "TextLayout.hpp"
#include "utf8_decode.hpp"
#include "utf8_parse.hpp"
#include <cstring>
#include <cstdlib>

// Direct access to the Unicode traits LUT for ASCII fast path.
// Defined in unicodetraits.cpp. For ASCII codepoints (< 0x80), indexing
// directly avoids the branch cascade in getTraitsForCodepoint().
extern const uint8_t _unicode_info_0000_33FF[];

size_t TextLayout::bytesForCodepoint(UNICODE_CODEPOINT cp) {
    if (cp <= 0x7F) return 1;
    if (cp <= 0x7FF) return 2;
    if (cp <= 0xFFFF) return 3;
    return 4;
}

WordWrapResult TextLayout::measureLineWrap(
    UNICODE_CODEPOINT* codepoints,
    size_t len,
    int16_t layoutWidth,
    uint8_t textSize,
    GlyphProvider* glyphProvider,
    int16_t initialCursorX
) {
    WordWrapResult result = {
        .codepointsConsumed = -1,
        .wrapped = false,
        .isParagraphBreak = false,
        .endCursorX = initialCursorX
    };

    if (len == 0 || glyphProvider == nullptr) {
        return result;
    }

    size_t wrapCandidate = 0;
    size_t position = 0;
    int16_t cursorX = initialCursorX;
    int16_t lastAdvance = 0;

    // Pre-fetch ASCII metrics cache to avoid virtual dispatch in the hot loop
    const Rect* asciiMetrics = glyphProvider->getAsciiMetricsCache();

    while (cursorX < layoutWidth) {
        // Check if we've consumed all input (no wrap needed)
        if (position >= len) {
            result.codepointsConsumed = -1;
            result.wrapped = false;
            result.isParagraphBreak = false;
            result.endCursorX = cursorX;
            return result;
        }

        UNICODE_CODEPOINT cp = codepoints[position];

        // Handle newline - this is a paragraph break, not a wrap
        if (cp == '\n') {
            result.codepointsConsumed = position + 1;
            result.wrapped = false;
            result.isParagraphBreak = true;
            result.endCursorX = 0;
            return result;
        }

        // Handle FF (form feed) — forced page break
        if (cp == 0x0C) {
            result.codepointsConsumed = position + 1;
            result.wrapped = false;
            result.isParagraphBreak = true;
            result.endCursorX = 0;
            return result;
        }

        // Handle .text format information separators (FS, GS, RS, US)
        // These act as line terminators, similar to newlines
        if (cp >= 0x1C && cp <= 0x1F) {
            result.codepointsConsumed = position + 1;
            result.wrapped = false;
            result.isParagraphBreak = true;
            result.endCursorX = 0;
            return result;
        }

        // Handle BS (backspace) — move cursor back for typewriter overprinting
        if (cp == 0x08) {
            cursorX -= lastAdvance;
            if (cursorX < initialCursorX) cursorX = initialCursorX;
            lastAdvance = 0;
            position++;
            continue;
        }

        // Skip other control characters (including SO/SI which have no width)
        if (cp < 0x20) {
            position++;
            continue;
        }

        unicode_info_t traits;
        Rect metrics;

        if (cp < 0x80) {
            // ASCII fast path: direct array lookups, no function calls
            traits.packed = _unicode_info_0000_33FF[cp];
            metrics = asciiMetrics[cp - 0x20];
        } else {
            traits = getTraitsForCodepoint(cp);
            metrics = glyphProvider->metricsForCodepoint(cp);
        }

        // Track potential wrap points (spaces, etc.)
        if (traits.is.linebreak) {
            wrapCandidate = position;
        }

        // Advance cursor for non-combining characters
        if (!(traits.is.nsm || traits.is.controlchar)) {
            int16_t advance = metrics.size.width * textSize;
            cursorX += advance;
            lastAdvance = advance;
        }

        position++;
    }

    // We exceeded the layout width - need to wrap
    result.wrapped = true;
    result.isParagraphBreak = false;
    result.endCursorX = 0;

    if (wrapCandidate > 0) {
        // Wrap at the last good break point (after the space)
        result.codepointsConsumed = wrapCandidate + 1;
    } else {
        // No good break point found - force break at current position
        result.codepointsConsumed = position;
    }

    return result;
}

int16_t TextLayout::getLineHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t lineSpacing) {
    return glyphProvider->getGlyphRowCount() * textSize + lineSpacing;
}

int16_t TextLayout::getParagraphHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t paragraphSpacing) {
    return glyphProvider->getGlyphRowCount() * textSize + paragraphSpacing;
}

int16_t TextLayout::calculateLineSpacing(GlyphProvider* glyphProvider) {
    return 2;
}

int16_t TextLayout::calculateParagraphSpacing(GlyphProvider* glyphProvider) {
    return glyphProvider->getGlyphRowCount() / 3;
}

int16_t TextLayout::measureTextWidth(const char* utf8String, uint8_t textSize, GlyphProvider* glyphProvider) {
    if (utf8String == nullptr || glyphProvider == nullptr || strlen(utf8String) == 0) {
        return 0;
    }

    size_t len = utf8_codepoint_length((char*)utf8String);
    if (len == 0) return 0;

    UNICODE_CODEPOINT* codepoints = (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (codepoints == nullptr) return 0;

    utf8_parse((char*)utf8String, codepoints);

    int16_t width = 0;
    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];

        // Skip control characters
        if (cp < 0x20) continue;

        unicode_info_t traits = getTraitsForCodepoint(cp);

        // Only count non-combining characters
        if (!(traits.is.nsm || traits.is.controlchar)) {
            Rect metrics = glyphProvider->metricsForCodepoint(cp);
            width += metrics.size.width * textSize;
        }
    }

    free(codepoints);
    return width;
}
