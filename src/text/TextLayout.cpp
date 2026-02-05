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
#include "utf8_decode.hpp"
#include "utf8_parse.hpp"
#include "esp_timer.h"
#include "esp_log.h"
#include <cstring>
#include <cstdlib>

// Performance counters for measureLineWrap internals
// These accumulate across calls and can be reset/read externally
static int64_t _perfTraitsLookupTime = 0;
static int64_t _perfMetricsLookupTime = 0;
static uint32_t _perfCodepointsProcessed = 0;

void TextLayout::resetPerfCounters() {
    _perfTraitsLookupTime = 0;
    _perfMetricsLookupTime = 0;
    _perfCodepointsProcessed = 0;
}

void TextLayout::getPerfCounters(int64_t& traitsTime, int64_t& metricsTime, uint32_t& codepoints) {
    traitsTime = _perfTraitsLookupTime;
    metricsTime = _perfMetricsLookupTime;
    codepoints = _perfCodepointsProcessed;
}

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
        .bytesConsumed = 0,
        .wrapped = false,
        .isParagraphBreak = false,
        .endCursorX = initialCursorX
    };

    if (len == 0 || glyphProvider == nullptr) {
        return result;
    }

    size_t wrapCandidate = 0;
    size_t wrapCandidateBytes = 0;
    size_t bytePosition = 0;
    size_t position = 0;
    int16_t cursorX = initialCursorX;

    while (cursorX < layoutWidth) {
        // Check if we've consumed all input (no wrap needed)
        if (position >= len) {
            result.codepointsConsumed = -1;
            result.bytesConsumed = bytePosition;
            result.wrapped = false;
            result.isParagraphBreak = false;
            result.endCursorX = cursorX;
            return result;
        }

        UNICODE_CODEPOINT cp = codepoints[position];
        _perfCodepointsProcessed++;

        // Handle newline - this is a paragraph break, not a wrap
        if (cp == '\n') {
            result.codepointsConsumed = position + 1;
            result.bytesConsumed = bytePosition + bytesForCodepoint(cp);
            result.wrapped = false;
            result.isParagraphBreak = true;
            result.endCursorX = 0;  // Line complete, next line starts at 0
            return result;
        }

        // Skip control characters but count their bytes
        if (cp < 0x20) {
            bytePosition += bytesForCodepoint(cp);
            position++;
            continue;
        }

        int64_t t0 = esp_timer_get_time();
        unicode_info_t traits = getTraitsForCodepoint(cp);
        int64_t t1 = esp_timer_get_time();
        _perfTraitsLookupTime += (t1 - t0);

        int64_t t2 = esp_timer_get_time();
        Rect metrics = glyphProvider->metricsForCodepoint(cp);
        int64_t t3 = esp_timer_get_time();
        _perfMetricsLookupTime += (t3 - t2);

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
    result.endCursorX = 0;  // Line complete, next line starts at 0

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
