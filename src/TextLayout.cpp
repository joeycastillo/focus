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

#include "TextLayout.hpp"

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
    GlyphProvider* glyphProvider
) {
    WordWrapResult result = {
        .codepointsConsumed = -1,
        .bytesConsumed = 0,
        .wrapped = false,
        .isParagraphBreak = false
    };

    if (len == 0 || glyphProvider == nullptr) {
        return result;
    }

    size_t wrapCandidate = 0;
    size_t wrapCandidateBytes = 0;
    size_t bytePosition = 0;
    size_t position = 0;
    int16_t cursorX = 0;

    while (cursorX < layoutWidth) {
        // Check if we've consumed all input (no wrap needed)
        if (position >= len) {
            result.codepointsConsumed = -1;
            result.bytesConsumed = bytePosition;
            result.wrapped = false;
            result.isParagraphBreak = false;
            return result;
        }

        UNICODE_CODEPOINT cp = codepoints[position];

        // Handle newline - this is a paragraph break, not a wrap
        if (cp == '\n') {
            result.codepointsConsumed = position + 1;
            result.bytesConsumed = bytePosition + bytesForCodepoint(cp);
            result.wrapped = false;
            result.isParagraphBreak = true;
            return result;
        }

        // Skip control characters but count their bytes
        if (cp < 0x20) {
            bytePosition += bytesForCodepoint(cp);
            position++;
            continue;
        }

        unicode_info_t traits = getTraitsForCodepoint(cp);
        Rect metrics = glyphProvider->metricsForCodepoint(cp);

        // Track potential wrap points (spaces, etc.)
        if (traits.is.linebreak) {
            wrapCandidate = position;
            wrapCandidateBytes = bytePosition;
        }

        // Advance cursor for non-combining characters
        if (!(traits.is.nsm || traits.is.controlchar)) {
            cursorX += metrics.size.width * textSize;
        }

        bytePosition += bytesForCodepoint(cp);
        position++;
    }

    // We exceeded the layout width - need to wrap
    result.wrapped = true;
    result.isParagraphBreak = false;

    if (wrapCandidate > 0) {
        // Wrap at the last good break point (after the space)
        result.codepointsConsumed = wrapCandidate + 1;
        result.bytesConsumed = wrapCandidateBytes + bytesForCodepoint(codepoints[wrapCandidate]);
    } else {
        // No good break point found - force break at current position
        result.codepointsConsumed = position;
        result.bytesConsumed = bytePosition;
    }

    return result;
}

int16_t TextLayout::getLineHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t lineSpacing) {
    return glyphProvider->getPointSize() * textSize + lineSpacing;
}

int16_t TextLayout::getParagraphHeight(GlyphProvider* glyphProvider, uint8_t textSize, int16_t paragraphSpacing) {
    return glyphProvider->getPointSize() * textSize + paragraphSpacing;
}

int16_t TextLayout::calculateLineSpacing(GlyphProvider* glyphProvider) {
    Point offset = glyphProvider->getOffset();
    return 2 - offset.y;
}

int16_t TextLayout::calculateParagraphSpacing(int16_t lineSpacing) {
    return 6 + lineSpacing;
}
