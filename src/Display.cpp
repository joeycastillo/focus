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

#include "Display.hpp"
#include "GlyphProvider.hpp"
#include "TextLayout.hpp"
#include "utf8_parse.hpp"
#include <string.h>

int Display::drawText(Rect layoutRect, int color, int text_size, const char * utf8String, GlyphProvider *glyphProvider) {
    if (glyphProvider == NULL) glyphProvider = this->defaultGlyphProvider.get();
    if (glyphProvider == NULL) return 0;
    if (strlen(utf8String) == 0) return 0;

    size_t len = utf8_codepoint_length((char *)utf8String);

    this->textSize = text_size;
    this->textColor = color;
    Point offset = glyphProvider->getOffset();
    this->lineSpacing = 2 - offset.y;
    this->paragraphSpacing = 6 + this->lineSpacing;
    this->layoutRect = layoutRect;
    this->glyphRowCount = glyphProvider->getGlyphRowCount();

    UNICODE_CODEPOINT *codepoints = (UNICODE_CODEPOINT *)malloc(len * sizeof(UNICODE_CODEPOINT));

    utf8_parse((char *)utf8String, codepoints);
    size_t retVal = this->writeCodepoints(codepoints, len, glyphProvider);
    free(codepoints);

    return retVal;
}

size_t Display::writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    size_t retVal = 0;
    size_t pos = 0;
    this->cursor = this->layoutRect.origin;

    while (pos < len) {
        WordWrapResult result = TextLayout::measureLineWrap(
            codepoints + pos,
            len - pos,
            this->layoutRect.size.width,
            this->textSize,
            glyphProvider
        );

        int32_t numGlyphsToDraw;
        if (result.codepointsConsumed < 0) {
            // No wrap needed - draw remaining text
            numGlyphsToDraw = (int32_t)(len - pos);
        } else {
            numGlyphsToDraw = result.codepointsConsumed;
        }

        for (size_t i = pos; i < pos + numGlyphsToDraw; i++) {
            retVal += this->writeCodepoint(codepoints[i], glyphProvider);
        }
        pos += numGlyphsToDraw;

        // Advance to next line if we wrapped (not for paragraph breaks - writeCodepoint handles those)
        if (result.wrapped) {
            this->cursor.y += glyphProvider->getPointSize() * this->textSize + this->lineSpacing;
            if (this->direction == 1) {
                this->cursor.x = this->layoutRect.origin.x;
            } else {
                this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width;
            }
        }

        if (this->cursor.y >= (this->layoutRect.origin.y + this->layoutRect.size.height)) break;
    }

    return retVal;
}

size_t Display::writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider) {
    // before we start, we don't need to fetch anything for control characters.
    if (codepoint == '\n' || codepoint == '\r') {
        this->cursor.y += this->glyphRowCount * this->textSize + this->paragraphSpacing;
        if (this->direction == 1) {
            this->cursor.x = this->layoutRect.origin.x;
        } else {
            this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width;
        }
        return 1;
    }
    // skip all control characters
    if (codepoint < 0x20) return 1;

    unicode_info_t traits = getTraitsForCodepoint(codepoint);
    Rect metrics = glyphProvider->metricsForCodepoint(codepoint);

    if (this->direction == 1 && traits.is.rtl) {
        direction = -1;
        uint8_t width = metrics.size.width;
        this->hasLastGlyph = false;
        this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width - width;
    }
    else if (this->direction == -1 && traits.is.ltr) {
        direction = 1;
        this->hasLastGlyph = false;
        this->cursor.x = this->layoutRect.origin.x;
    }

    uint8_t *glyph = glyphProvider->glyphForCodepoint(codepoint);
    // word wrap should go here
    if (traits.is.nsm && this->hasLastGlyph) {
        // Draw over the last glyph, and do not add to advance
        drawGlyph(this->lastGlyphPosition.x, this->lastGlyphPosition.y, metrics, traits, glyph);
    } else {
        // stash current cursor position
        this->hasLastGlyph = true;
        this->lastGlyphPosition = this->cursor;
        // draw glyph
        int advance = drawGlyph(this->cursor.x, this->cursor.y, metrics, traits, glyph);
        // advance cursor
        this->cursor.x += advance * this->direction;
    }

    return 1;
}

int Display::drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph) {
    uint8_t width = glyphRect.size.width;
    uint8_t bytesPerRow = (width + 7) / 8;
    bool mirrored = (this->direction == -1) && traits.is.mirrored;

    // General loop that handles any glyph width (1, 2, 3+ bytes per row)
    // Glyph data is stored as: [row0_byte0, row0_byte1, ..., row1_byte0, row1_byte1, ...]
    // Bit order: MSB is leftmost pixel, LSB is rightmost pixel within each byte
    for (int row = 0; row < this->glyphRowCount; row++) {
        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            uint8_t line = glyph[row * bytesPerRow + byteIdx];
            int xOffset = byteIdx * 8;

            // j counts down from 7 to 0, line shifts right each iteration
            // When j=7, we check bit 0 (LSB) -> rightmost pixel at xOffset+7
            // When j=0, we check bit 7 (MSB) -> leftmost pixel at xOffset+0
            for (int8_t j = 7; j >= 0; j--, line >>= 1) {
                if (line & 1) {
                    int pixelX = mirrored ? (width - 1 - (xOffset + j)) : (xOffset + j);
                    if (this->textSize == 1) {
                        drawPixel(x + pixelX, y + row, this->textColor);
                    } else {
                        this->fillRect(x + pixelX * this->textSize, y + row * this->textSize,
                                      this->textSize, this->textSize, this->textColor);
                    }
                }
            }
        }
    }

    return width * this->textSize;
}

void Display::setDefaultGlyphProvider(std::shared_ptr<GlyphProvider> glyphProvider) {
    this->defaultGlyphProvider = glyphProvider;
}
