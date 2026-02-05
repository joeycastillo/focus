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

#include "CanvasView.hpp"
#include "Display.hpp"
#include "TextLayout.hpp"
#include "utf8_parse.hpp"
#include <algorithm>
#include <cstring>

extern const uint8_t _unicode_info_0000_33FF[];

CanvasView::CanvasView(Rect rect)
    : View(rect),
      rowBytes((rect.size.width + 7) / 8),
      buffer(((rect.size.width + 7) / 8) * rect.size.height, 0xFF) {
    this->opaque = true;
}

void CanvasView::draw(int x, int y) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                            this->frame.size.width, this->frame.size.height,
                            this->buffer.data(), this->rowBytes);
    }

    // Draw subviews on top
    int subviewX = x + this->frame.origin.x - this->bounds.origin.x;
    int subviewY = y + this->frame.origin.y - this->bounds.origin.y;
    for (std::shared_ptr<View> view : this->subviews) {
        if (!view->isHidden()) view->draw(subviewX, subviewY);
    }
}

void CanvasView::drawPixel(int x, int y, int color) {
    if (x < 0 || x >= frame.size.width || y < 0 || y >= frame.size.height) return;
    int idx = y * rowBytes + (x >> 3);
    uint8_t mask = 0x80 >> (x & 7);
    if (color == 0) {
        buffer[idx] &= ~mask;  // black: clear bit
    } else {
        buffer[idx] |= mask;   // white: set bit
    }
}

void CanvasView::drawRect(int x, int y, int w, int h, int color) {
    for (int i = x; i < x + w; i++) {
        drawPixel(i, y, color);
        drawPixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        drawPixel(x, j, color);
        drawPixel(x + w - 1, j, color);
    }
}

void CanvasView::fillRect(int x, int y, int w, int h, int color) {
    // Clamp to canvas bounds
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min((int)frame.size.width, x + w);
    int y1 = std::min((int)frame.size.height, y + h);
    if (x0 >= x1 || y0 >= y1) return;

    uint8_t fillByte = (color == 0) ? 0x00 : 0xFF;
    int firstByte = x0 >> 3;
    int lastByte = (x1 - 1) >> 3;
    int startBit = x0 & 7;
    int endBit = (x1 - 1) & 7;

    for (int row = y0; row < y1; row++) {
        int rowOffset = row * rowBytes;

        if (firstByte == lastByte) {
            // All bits within a single byte
            uint8_t mask = (0xFF >> startBit) & (0xFF << (7 - endBit));
            if (color == 0) {
                buffer[rowOffset + firstByte] &= ~mask;
            } else {
                buffer[rowOffset + firstByte] |= mask;
            }
        } else {
            // First partial byte
            if (startBit > 0) {
                uint8_t mask = 0xFF >> startBit;
                if (color == 0) {
                    buffer[rowOffset + firstByte] &= ~mask;
                } else {
                    buffer[rowOffset + firstByte] |= mask;
                }
            }
            // Middle full bytes
            int midStart = firstByte + (startBit > 0 ? 1 : 0);
            if (lastByte > midStart) {
                std::memset(&buffer[rowOffset + midStart], fillByte, lastByte - midStart);
            }
            // Last partial byte
            uint8_t endMask = 0xFF << (7 - endBit);
            if (color == 0) {
                buffer[rowOffset + lastByte] &= ~endMask;
            } else {
                buffer[rowOffset + lastByte] |= endMask;
            }
        }
    }
}

void CanvasView::drawCircle(int cx, int cy, int r, int color) {
    int x = r, y = 0;
    int d = 1 - r;
    while (x >= y) {
        drawPixel(cx + x, cy + y, color);
        drawPixel(cx - x, cy + y, color);
        drawPixel(cx + x, cy - y, color);
        drawPixel(cx - x, cy - y, color);
        drawPixel(cx + y, cy + x, color);
        drawPixel(cx - y, cy + x, color);
        drawPixel(cx + y, cy - x, color);
        drawPixel(cx - y, cy - x, color);
        y++;
        if (d <= 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
}

void CanvasView::fillCircle(int cx, int cy, int r, int color) {
    int x = r, y = 0;
    int d = 1 - r;
    while (x >= y) {
        fillRect(cx - x, cy + y, 2 * x + 1, 1, color);
        fillRect(cx - x, cy - y, 2 * x + 1, 1, color);
        fillRect(cx - y, cy + x, 2 * y + 1, 1, color);
        fillRect(cx - y, cy - x, 2 * y + 1, 1, color);
        y++;
        if (d <= 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
}

void CanvasView::clear(int color) {
    std::memset(buffer.data(), (color == 0) ? 0x00 : 0xFF, buffer.size());
}

// --- Font and text rendering ---

void CanvasView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
}

int CanvasView::drawText(Rect layoutRect, int color, int text_size, const char *utf8String,
                         TextAlignment alignment) {
    GlyphProvider *glyphProvider = nullptr;
    if (this->font) {
        glyphProvider = this->font->getGlyphProvider();
    } else {
        auto sys = Font::systemFont();
        if (sys) glyphProvider = sys->getGlyphProvider();
    }
    if (glyphProvider == nullptr) return 0;
    if (strlen(utf8String) == 0) return 0;

    size_t len = utf8_codepoint_length((char *)utf8String);

    this->textSize = text_size;
    this->textColor = color;
    this->glyphRowCount = glyphProvider->getGlyphRowCount();
    this->lineSpacing = TextLayout::calculateLineSpacing(glyphProvider);
    this->paragraphSpacing = TextLayout::calculateParagraphSpacing(glyphProvider);
    this->textLayoutRect = layoutRect;
    this->textAlignment = alignment;
    this->direction = 1;
    this->hasLastGlyph = false;

    UNICODE_CODEPOINT *codepoints = (UNICODE_CODEPOINT *)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (!codepoints) return 0;

    utf8_parse((char *)utf8String, codepoints);
    size_t retVal = this->writeCodepoints(codepoints, len, glyphProvider);
    free(codepoints);

    return retVal;
}

int16_t CanvasView::measureCodepointsWidth(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    int16_t width = 0;
    const Rect* asciiMetrics = glyphProvider->getAsciiMetricsCache();
    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];
        if (cp < 0x20) continue;
        unicode_info_t traits;
        Rect metrics;
        if (cp < 0x80) {
            traits.packed = _unicode_info_0000_33FF[cp];
            metrics = asciiMetrics[cp - 0x20];
        } else {
            traits = getTraitsForCodepoint(cp);
            metrics = glyphProvider->metricsForCodepoint(cp);
        }
        if (!(traits.is.nsm || traits.is.controlchar)) {
            width += metrics.size.width * this->textSize;
        }
    }
    return width;
}

size_t CanvasView::writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    size_t retVal = 0;
    size_t pos = 0;
    this->cursor = this->textLayoutRect.origin;

    while (pos < len) {
        WordWrapResult result = TextLayout::measureLineWrap(
            codepoints + pos,
            len - pos,
            this->textLayoutRect.size.width,
            this->textSize,
            glyphProvider
        );

        int32_t numGlyphsToDraw;
        if (result.codepointsConsumed < 0) {
            numGlyphsToDraw = (int32_t)(len - pos);
        } else {
            numGlyphsToDraw = result.codepointsConsumed;
        }

        // Apply text alignment offset for this line
        if (this->textAlignment != TextAlignmentLeft && this->direction == 1) {
            int16_t lineWidth = measureCodepointsWidth(codepoints + pos, numGlyphsToDraw, glyphProvider);
            int16_t slack = this->textLayoutRect.size.width - lineWidth;
            if (slack > 0) {
                if (this->textAlignment == TextAlignmentCenter) {
                    this->cursor.x = this->textLayoutRect.origin.x + slack / 2;
                } else if (this->textAlignment == TextAlignmentRight) {
                    this->cursor.x = this->textLayoutRect.origin.x + slack;
                }
            }
        }

        for (size_t i = pos; i < pos + numGlyphsToDraw; i++) {
            retVal += this->writeCodepoint(codepoints[i], glyphProvider);
        }
        pos += numGlyphsToDraw;

        if (result.wrapped) {
            this->cursor.y += TextLayout::getLineHeight(glyphProvider, this->textSize, this->lineSpacing);
            if (this->direction == 1) {
                this->cursor.x = this->textLayoutRect.origin.x;
            } else {
                this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
            }
        }

        // Also handle paragraph breaks (newlines)
        if (result.isParagraphBreak) {
            this->cursor.x = this->textLayoutRect.origin.x;
        }

        if (this->cursor.y >= (this->textLayoutRect.origin.y + this->textLayoutRect.size.height)) break;
    }

    return retVal;
}

size_t CanvasView::writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider) {
    if (codepoint == '\n' || codepoint == '\r') {
        this->cursor.y += TextLayout::getParagraphHeight(glyphProvider, this->textSize, this->paragraphSpacing);
        if (this->direction == 1) {
            this->cursor.x = this->textLayoutRect.origin.x;
        } else {
            this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
        }
        return 1;
    }
    if (codepoint < 0x20) return 1;

    unicode_info_t traits = getTraitsForCodepoint(codepoint);
    Rect metrics = glyphProvider->metricsForCodepoint(codepoint);

    if (this->direction == 1 && traits.is.rtl) {
        direction = -1;
        uint8_t width = metrics.size.width;
        this->hasLastGlyph = false;
        this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width - width;
    } else if (this->direction == -1 && traits.is.ltr) {
        direction = 1;
        this->hasLastGlyph = false;
        this->cursor.x = this->textLayoutRect.origin.x;
    }

    uint8_t *glyph = glyphProvider->glyphForCodepoint(codepoint);
    if (traits.is.nsm && this->hasLastGlyph) {
        drawGlyph(this->lastGlyphPosition.x, this->lastGlyphPosition.y, metrics, traits, glyph);
    } else {
        this->hasLastGlyph = true;
        this->lastGlyphPosition = this->cursor;
        int advance = drawGlyph(this->cursor.x, this->cursor.y, metrics, traits, glyph);
        this->cursor.x += advance * this->direction;
    }

    return 1;
}

int CanvasView::drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph) {
    uint8_t width = glyphRect.size.width;
    uint8_t bytesPerRow = (width + 7) / 8;
    bool mirrored = (this->direction == -1) && traits.is.mirrored;
    int bbxOffset = glyphRect.origin.x;

    for (int row = 0; row < this->glyphRowCount; row++) {
        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            uint8_t line = glyph[row * bytesPerRow + byteIdx];
            int xOffset = byteIdx * 8;

            for (int8_t j = 7; j >= 0; j--, line >>= 1) {
                if (line & 1) {
                    int pixelX = mirrored ? (width - 1 - (xOffset + j)) : (xOffset + j);
                    if (this->textSize == 1) {
                        drawPixel(x + bbxOffset + pixelX, y + row, this->textColor);
                    } else {
                        fillRect((x + bbxOffset + pixelX) * this->textSize, y + row * this->textSize,
                                 this->textSize, this->textSize, this->textColor);
                    }
                }
            }
        }
    }

    return width * this->textSize;
}
