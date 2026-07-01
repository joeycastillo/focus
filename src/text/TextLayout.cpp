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

namespace focus {

// Direct access to the Unicode traits LUT for ASCII fast path.
// Defined in UnicodeTraits.cpp. For ASCII codepoints (< 0x80), indexing
// directly avoids the branch cascade in getTraitsForCodepoint().
extern const uint16_t _unicode_info_0000_33FF[];

size_t TextLayout::bytesForCodepoint(UNICODE_CODEPOINT cp) {
    if (cp <= 0x7F) return 1;
    if (cp <= 0x7FF) return 2;
    if (cp <= 0xFFFF) return 3;
    return 4;
}

bool applyEmphasisShift(UNICODE_CODEPOINT cp, uint8_t& depth) {
    if (cp == TextControlCode::EmphasisIncrease) {
        depth = depth < 3 ? depth + 1 : 3;
        return true;
    }
    if (cp == TextControlCode::EmphasisDecrease) {
        depth = depth > 0 ? depth - 1 : 0;
        return true;
    }
    return false;
}

WordWrapResult TextLayout::measureLineWrap(
    UNICODE_CODEPOINT* codepoints,
    size_t len,
    int16_t layoutWidth,
    uint8_t textSize,
    const GlyphProvider* glyphProvider,
    int16_t initialCursorX,
    FontStyle initialEmphasis,
    const Hyphenator* hyphenator
) {
    WordWrapResult result = {
        .codepointsConsumed = -1,
        .wrapped = false,
        .isParagraphBreak = false,
        .needsHyphen = false,
        .endCursorX = initialCursorX
    };

    if (len == 0 || glyphProvider == nullptr) {
        return result;
    }

    size_t wrapCandidate = 0;
    size_t position = 0;
    int16_t cursorX = initialCursorX;
    int16_t lastAdvance = 0;
    uint8_t emphasis = static_cast<uint8_t>(initialEmphasis);

    // Pre-fetch ASCII metrics cache for the Regular fast path. When emphasis is
    // non-regular and the provider supports it, we go through metricsForCodepoint()
    // instead to get style-accurate widths.
    const GlyphMetrics* asciiMetrics = glyphProvider->getAsciiMetricsCache();
    bool emphasisAware = initialEmphasis != FontStyle::Regular || glyphProvider->supportsEmphasis(FontStyle::Italic)
                                                              || glyphProvider->supportsEmphasis(FontStyle::Bold);

    while (cursorX <= layoutWidth) {
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

        // Handle FF (form feed) — stop the line so the caller can
        // process the page break via its style run.
        if (cp == TextControlCode::PageBreak) {
            result.codepointsConsumed = position + 1;
            result.wrapped = false;
            result.isParagraphBreak = true;
            result.endCursorX = 0;
            return result;
        }

        // Handle BS (backspace) — move cursor back for typewriter overprinting
        if (cp == TextControlCode::Backspace) {
            cursorX -= lastAdvance;
            if (cursorX < initialCursorX) cursorX = initialCursorX;
            lastAdvance = 0;
            position++;
            continue;
        }

        // Track SO/SI emphasis changes (no width, just state)
        if (applyEmphasisShift(cp, emphasis)) {
            position++;
            continue;
        }

        // Skip other control characters
        if (cp < 0x20) {
            position++;
            continue;
        }

        unicode_info_t traits;
        GlyphMetrics metrics;

        if (cp < 0x80 && (!emphasisAware || emphasis == 0)) {
            // ASCII fast path: direct array lookups, no function calls.
            // Only valid when emphasis is 0 (regular metrics cached).
            traits.packed = _unicode_info_0000_33FF[cp];
            metrics = asciiMetrics[cp - 0x20];
        } else {
            if (cp < 0x80) {
                traits.packed = _unicode_info_0000_33FF[cp];
            } else {
                traits = getTraitsForCodepoint(cp);
            }
            metrics = glyphProvider->metricsForCodepoint(cp, static_cast<FontStyle>(emphasis));
        }

        // Advance cursor for non-combining characters
        if (!(traits.is.nsm || traits.is.controlchar)) {
            int16_t advance = metrics.advance * textSize;
            cursorX += advance;
            lastAdvance = advance;
        }

        // Track potential wrap points — only if this character fits on the line.
        // Must come AFTER the advance so we don't register an overflowing
        // character as a wrap candidate (causes CJK right-edge clipping).
        if (traits.is.linebreak && cursorX <= layoutWidth) {
            wrapCandidate = position;
        }

        position++;
    }

    // We exceeded the layout width - need to wrap
    result.wrapped = true;
    result.isParagraphBreak = false;
    result.endCursorX = 0;

    // Try hyphenation on the overflowing word before falling back to word wrap.
    // The overflowing word starts after the last wrap candidate (space) and extends
    // to the current position. We extract it, find legal break positions, and check
    // if any prefix + hyphen fits within the layout width.
    if (hyphenator != nullptr) {
        // Identify the overflowing word boundaries in the codepoint array
        size_t wordStart = (wrapCandidate > 0) ? wrapCandidate + 1 : 0;
        // Skip any leading control characters (SO/SI/BS) that aren't part of the word
        while (wordStart < position && codepoints[wordStart] < 0x20) {
            wordStart++;
        }
        size_t wordEnd = position; // one past the last codepoint we processed
        // Trim trailing control characters
        while (wordEnd > wordStart && codepoints[wordEnd - 1] < 0x20) {
            wordEnd--;
        }
        size_t wordLen = wordEnd - wordStart;

        if (wordLen >= 4) {
            size_t breakPositions[32];
            size_t breakCount = hyphenator->findBreakPositions(
                codepoints + wordStart, wordLen, breakPositions, 32);

            if (breakCount > 0) {
                // Walk break positions from rightmost to leftmost (greedy: fill as much as possible)
                for (int bi = (int)breakCount - 1; bi >= 0; bi--) {
                    // breakPositions[bi] is the index within the word of the last codepoint in the prefix.
                    // The split point in the full codepoint array is wordStart + breakPositions[bi] + 1.
                    size_t splitCodepoint = wordStart + breakPositions[bi] + 1;

                    // Measure width from line start to split point + hyphen
                    int16_t prefixWidth = initialCursorX;
                    uint8_t emph = static_cast<uint8_t>(initialEmphasis);
                    for (size_t k = 0; k < splitCodepoint; k++) {
                        UNICODE_CODEPOINT cp = codepoints[k];
                        if (applyEmphasisShift(cp, emph)) continue;
                        if (cp == TextControlCode::Backspace) {
                            // Backspace
                            GlyphMetrics prevMetrics = glyphProvider->metricsForCodepoint(
                                k > 0 ? codepoints[k-1] : ' ', static_cast<FontStyle>(emph));
                            prefixWidth -= prevMetrics.advance * textSize;
                            if (prefixWidth < initialCursorX) prefixWidth = initialCursorX;
                            continue;
                        }
                        if (cp < 0x20) continue;

                        unicode_info_t traits;
                        if (cp < 0x80) {
                            traits.packed = _unicode_info_0000_33FF[cp];
                        } else {
                            traits = getTraitsForCodepoint(cp);
                        }
                        if (!(traits.is.nsm || traits.is.controlchar)) {
                            GlyphMetrics metrics = glyphProvider->metricsForCodepoint(cp, static_cast<FontStyle>(emph));
                            prefixWidth += metrics.advance * textSize;
                        }
                    }

                    // Measure hyphen at the emphasis state at this break position
                    int16_t hyphenAdvance = glyphProvider->metricsForCodepoint('-', static_cast<FontStyle>(emph)).advance * textSize;

                    if (prefixWidth + hyphenAdvance <= layoutWidth) {
                        result.codepointsConsumed = splitCodepoint;
                        result.needsHyphen = true;
                        return result;
                    }
                }
            }
        }
    }

    if (wrapCandidate > 0) {
        // Wrap at the last good break point (after the space)
        result.codepointsConsumed = wrapCandidate + 1;
    } else {
        // No good break point found - force break at current position
        result.codepointsConsumed = position;
    }

    return result;
}

int16_t TextLayout::getLineHeight(const GlyphProvider* glyphProvider, uint8_t textSize, int16_t lineSpacing) {
    return glyphProvider->getGlyphRowCount() * textSize + lineSpacing;
}

int16_t TextLayout::getParagraphHeight(const GlyphProvider* glyphProvider, uint8_t textSize, int16_t paragraphSpacing) {
    return glyphProvider->getGlyphRowCount() * textSize + paragraphSpacing;
}

int16_t TextLayout::calculateLineSpacing(const GlyphProvider* glyphProvider) {
    return 2;
}

int16_t TextLayout::calculateParagraphSpacing(const GlyphProvider* glyphProvider) {
    return glyphProvider->getGlyphRowCount() / 3;
}

int16_t TextLayout::measureTextWidth(const char* utf8String, uint8_t textSize, const GlyphProvider* glyphProvider) {
    if (utf8String == nullptr || glyphProvider == nullptr || strlen(utf8String) == 0) {
        return 0;
    }

    size_t len = utf8_codepoint_length((char*)utf8String);
    if (len == 0) return 0;

    UNICODE_CODEPOINT* codepoints = (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (codepoints == nullptr) return 0;

    utf8_parse((char*)utf8String, codepoints);

    int16_t width = 0;
    uint8_t emphasis = 0;
    bool emphasisAware = glyphProvider->supportsEmphasis(FontStyle::Italic) || glyphProvider->supportsEmphasis(FontStyle::Bold);
    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];

        // Track SO/SI emphasis changes
        if (applyEmphasisShift(cp, emphasis)) continue;

        // Skip other control characters
        if (cp < 0x20) continue;

        unicode_info_t traits = getTraitsForCodepoint(cp);

        // Only count non-combining characters
        if (!(traits.is.nsm || traits.is.controlchar)) {
            GlyphMetrics metrics = emphasisAware
                ? glyphProvider->metricsForCodepoint(cp, static_cast<FontStyle>(emphasis))
                : glyphProvider->metricsForCodepoint(cp);
            width += metrics.advance * textSize;
        }
    }

    free(codepoints);
    return width;
}

int16_t TextLayout::measureTextHeight(const char* utf8String, int16_t layoutWidth, uint8_t textSize, const GlyphProvider* glyphProvider) {
    if (utf8String == nullptr || glyphProvider == nullptr || strlen(utf8String) == 0 || layoutWidth <= 0) {
        return 0;
    }

    size_t len = utf8_codepoint_length((char*)utf8String);
    if (len == 0) return 0;

    UNICODE_CODEPOINT* codepoints = (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (codepoints == nullptr) return 0;

    utf8_parse((char*)utf8String, codepoints);

    int16_t lineSpacing = calculateLineSpacing(glyphProvider);
    int16_t paragraphSpacing = calculateParagraphSpacing(glyphProvider);
    int16_t lineHeight = getLineHeight(glyphProvider, textSize, lineSpacing);
    int16_t paragraphHeight = getParagraphHeight(glyphProvider, textSize, paragraphSpacing);

    int16_t totalHeight = 0;
    size_t offset = 0;
    uint8_t emphasis = 0;

    while (offset < len) {
        WordWrapResult result = measureLineWrap(
            codepoints + offset,
            len - offset,
            layoutWidth,
            textSize,
            glyphProvider,
            0,
            static_cast<FontStyle>(emphasis));

        if (result.codepointsConsumed < 0) {
            // Remaining text fits on one line — this is the last line
            totalHeight += glyphProvider->getGlyphRowCount() * textSize;
            break;
        }

        // Track SO/SI emphasis changes through consumed codepoints
        for (int32_t i = 0; i < result.codepointsConsumed; i++) {
            applyEmphasisShift(codepoints[offset + i], emphasis);
        }

        if (result.isParagraphBreak) {
            totalHeight += paragraphHeight;
        } else {
            totalHeight += lineHeight;
        }

        offset += result.codepointsConsumed;
    }

    free(codepoints);
    return totalHeight;
}

}  // namespace focus
