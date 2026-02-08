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
#include "ArabicShaping.hpp"
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

void CanvasView::setCanvasMode(DisplayMode mode) {
    canvasMode = mode;
    if (mode == DisplayMode::TwoBpp) {
        buffer1.resize(rowBytes * frame.size.height, 0xFF);
    } else {
        buffer1.clear();
        buffer1.shrink_to_fit();
    }
}

void CanvasView::draw(int x, int y) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        if (canvasMode == DisplayMode::TwoBpp) {
            display->blitOpaque2bpp(x + this->frame.origin.x, y + this->frame.origin.y,
                                    this->frame.size.width, this->frame.size.height,
                                    this->buffer.data(), this->buffer1.data(),
                                    this->rowBytes);
        } else {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->buffer.data(), this->rowBytes);
        }
    }

    // Draw subviews on top
    int subviewX = x + this->frame.origin.x - this->bounds.origin.x;
    int subviewY = y + this->frame.origin.y - this->bounds.origin.y;
    for (std::shared_ptr<View> view : this->subviews) {
        if (!view->isHidden()) view->draw(subviewX, subviewY);
    }
}

void CanvasView::drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= frame.size.width || y < 0 || y >= frame.size.height) return;
    int idx = y * rowBytes + (x >> 3);
    uint8_t mask = 0x80 >> (x & 7);

    if (canvasMode == DisplayMode::TwoBpp) {
        // plane0 (buffer) stores bit 1 (high bit) of color
        if (color & 0x02) {
            buffer[idx] |= mask;
        } else {
            buffer[idx] &= ~mask;
        }
        // plane1 (buffer1) stores bit 0 (low bit) of color
        if (color & 0x01) {
            buffer1[idx] |= mask;
        } else {
            buffer1[idx] &= ~mask;
        }
    } else {
        if (color == 0) {
            buffer[idx] &= ~mask;  // black: clear bit
        } else {
            buffer[idx] |= mask;   // white: set bit
        }
    }
}

void CanvasView::drawRect(int x, int y, int w, int h, uint16_t color) {
    for (int i = x; i < x + w; i++) {
        drawPixel(i, y, color);
        drawPixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        drawPixel(x, j, color);
        drawPixel(x + w - 1, j, color);
    }
}

void CanvasView::_fillPlane(uint8_t* plane, int x0, int y0, int x1, int y1, uint8_t fillByte) {
    int firstByte = x0 >> 3;
    int lastByte = (x1 - 1) >> 3;
    int startBit = x0 & 7;
    int endBit = (x1 - 1) & 7;
    bool setBits = (fillByte != 0);

    for (int row = y0; row < y1; row++) {
        int rowOffset = row * rowBytes;

        if (firstByte == lastByte) {
            uint8_t mask = (0xFF >> startBit) & (0xFF << (7 - endBit));
            if (setBits) {
                plane[rowOffset + firstByte] |= mask;
            } else {
                plane[rowOffset + firstByte] &= ~mask;
            }
        } else {
            if (startBit > 0) {
                uint8_t mask = 0xFF >> startBit;
                if (setBits) {
                    plane[rowOffset + firstByte] |= mask;
                } else {
                    plane[rowOffset + firstByte] &= ~mask;
                }
            }
            int midStart = firstByte + (startBit > 0 ? 1 : 0);
            if (lastByte > midStart) {
                std::memset(&plane[rowOffset + midStart], fillByte, lastByte - midStart);
            }
            uint8_t endMask = 0xFF << (7 - endBit);
            if (setBits) {
                plane[rowOffset + lastByte] |= endMask;
            } else {
                plane[rowOffset + lastByte] &= ~endMask;
            }
        }
    }
}

void CanvasView::fillRect(int x, int y, int w, int h, uint16_t color) {
    // Clamp to canvas bounds
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min((int)frame.size.width, x + w);
    int y1 = std::min((int)frame.size.height, y + h);
    if (x0 >= x1 || y0 >= y1) return;

    if (canvasMode == DisplayMode::TwoBpp) {
        _fillPlane(buffer.data(), x0, y0, x1, y1, (color & 0x02) ? 0xFF : 0x00);
        _fillPlane(buffer1.data(), x0, y0, x1, y1, (color & 0x01) ? 0xFF : 0x00);
    } else {
        _fillPlane(buffer.data(), x0, y0, x1, y1, (color != 0) ? 0xFF : 0x00);
    }
}

void CanvasView::drawCircle(int cx, int cy, int r, uint16_t color) {
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

void CanvasView::fillCircle(int cx, int cy, int r, uint16_t color) {
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

void CanvasView::clear(uint16_t color) {
    if (canvasMode == DisplayMode::TwoBpp) {
        std::memset(buffer.data(), (color & 0x02) ? 0xFF : 0x00, buffer.size());
        std::memset(buffer1.data(), (color & 0x01) ? 0xFF : 0x00, buffer1.size());
    } else {
        std::memset(buffer.data(), (color != 0) ? 0xFF : 0x00, buffer.size());
    }
}

// --- Font and text rendering ---

void CanvasView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
}

void CanvasView::setArabicShaping(bool enabled) {
    this->arabicShaping = enabled;
}

int CanvasView::drawText(Rect layoutRect, uint16_t color, int text_size, const char *utf8String,
                         TextAlignment alignment, int initialEmphasisDepth, int initialIndentLevel) {
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
    this->emphasisDepth = initialEmphasisDepth;
    this->readingTitle = false;
    this->lastWasNewline = false;
    this->initialIndentLevel = initialIndentLevel;

    UNICODE_CODEPOINT *codepoints = (UNICODE_CODEPOINT *)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (!codepoints) return 0;

    utf8_parse((char *)utf8String, codepoints);
    if (this->arabicShaping) {
        shapeArabic(codepoints, len);
    }
    size_t retVal = this->writeCodepoints(codepoints, len, glyphProvider);
    free(codepoints);

    return retVal;
}

int16_t CanvasView::measureCodepointsWidth(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    int16_t width = 0;
    int16_t lastAdvance = 0;
    const Rect* asciiMetrics = glyphProvider->getAsciiMetricsCache();
    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];
        if (cp == 0x08) {
            width -= lastAdvance;
            if (width < 0) width = 0;
            lastAdvance = 0;
            continue;
        }
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
            int16_t advance = metrics.size.width * this->textSize;
            width += advance;
            lastAdvance = advance;
        }
    }
    return width;
}

size_t CanvasView::writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    size_t retVal = 0;
    size_t pos = 0;
    this->cursor = this->textLayoutRect.origin;

    // Block quote indentation state
    bool atLineStart = true;
    int16_t currentIndent = 0;
    Rect spaceMetrics = glyphProvider->metricsForCodepoint(' ');
    int16_t indentPerLevel = spaceMetrics.size.width * this->textSize * 3;

    // Apply initial indent from page break record (for mid-paragraph starts)
    if (this->initialIndentLevel > 0) {
        currentIndent = this->initialIndentLevel * indentPerLevel;
        atLineStart = false;
    }

    while (pos < len) {
        // Scan for DLE+> prefix at the start of a logical line
        if (atLineStart) {
            int indentLevel = 0;
            while (pos + 1 < len &&
                   codepoints[pos] == 0x10 &&
                   codepoints[pos + 1] == '>') {
                indentLevel++;
                pos += 2;
                retVal += 2;
            }
            currentIndent = indentLevel * indentPerLevel;
        }

        int16_t effectiveWidth = this->textLayoutRect.size.width - 2 * currentIndent;
        int16_t indentedOriginX = this->textLayoutRect.origin.x + currentIndent;

        // Set cursor to indented position for this line
        if (this->direction == 1) {
            this->cursor.x = indentedOriginX;
        }

        WordWrapResult result = TextLayout::measureLineWrap(
            codepoints + pos,
            len - pos,
            effectiveWidth,
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
            int16_t slack = effectiveWidth - lineWidth;
            if (slack > 0) {
                if (this->textAlignment == TextAlignmentCenter) {
                    this->cursor.x = indentedOriginX + slack / 2;
                } else if (this->textAlignment == TextAlignmentRight) {
                    this->cursor.x = indentedOriginX + slack;
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
                this->cursor.x = indentedOriginX;
            } else {
                this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width - currentIndent;
            }
            atLineStart = false; // Word-wrap continuation inherits indent
        }

        // Also handle paragraph breaks (newlines)
        if (result.isParagraphBreak) {
            if (this->direction == 1) {
                this->cursor.x = this->textLayoutRect.origin.x;
            } else {
                this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
            }
            atLineStart = true; // Next line will scan for its own DLE+> prefix
        }

        if (this->cursor.y >= (this->textLayoutRect.origin.y + this->textLayoutRect.size.height)) break;
    }

    return retVal;
}

size_t CanvasView::writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider) {
    if (codepoint == '\r') return 1; // Ignore CR; LF handles line breaks

    if (codepoint == '\n') {
        if (this->readingTitle) {
            this->readingTitle = false;
            this->emphasisDepth = this->savedEmphasisDepth;
            // Extra spacing after title line
            this->cursor.y += TextLayout::getLineHeight(glyphProvider, this->textSize, this->lineSpacing) + this->paragraphSpacing;
        } else if (this->lastWasNewline) {
            // Consecutive newline = paragraph break, just add paragraph spacing
            this->cursor.y += this->paragraphSpacing;
        } else {
            // Single newline = line break (same as word wrap)
            this->cursor.y += TextLayout::getLineHeight(glyphProvider, this->textSize, this->lineSpacing);
        }
        this->lastWasNewline = true;
        if (this->direction == 1) {
            this->cursor.x = this->textLayoutRect.origin.x;
        } else {
            this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
        }
        return 1;
    }

    // .text format: SO (Shift Out) increases emphasis depth
    if (codepoint == 0x0E) {
        this->emphasisDepth = std::min(this->emphasisDepth + 1, 3);
        return 1;
    }
    // .text format: SI (Shift In) decreases emphasis depth
    if (codepoint == 0x0F) {
        this->emphasisDepth = std::max(this->emphasisDepth - 1, 0);
        return 1;
    }
    // .text format: BS (0x08) — backspace for typewriter overprinting
    // Move cursor back to the last glyph position so the next character overprints
    if (codepoint == 0x08) {
        if (this->hasLastGlyph) {
            this->cursor.x = this->lastGlyphPosition.x;
        }
        return 1;
    }
    // .text format: FS/GS/RS (0x1C–0x1E) — chapter separator, enter title mode
    if (codepoint >= 0x1C && codepoint <= 0x1E) {
        this->readingTitle = true;
        this->savedEmphasisDepth = this->emphasisDepth;
        this->emphasisDepth = 2; // render title bold
        return 1;
    }
    // .text format: FF (0x0C) — forced page break
    // Paginator places page breaks here; push cursor past layout to end rendering
    if (codepoint == 0x0C) {
        this->cursor.y = this->textLayoutRect.origin.y + this->textLayoutRect.size.height;
        return 1;
    }
    // .text format: US (0x1F) — scene break, add vertical whitespace
    if (codepoint == 0x1F) {
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
    if (traits.is.controlchar) return 1;

    this->lastWasNewline = false; // Visible character breaks consecutive newline tracking

    Rect metrics = glyphProvider->metricsForCodepoint(codepoint);

    if (this->direction == 1 && traits.is.rtl) {
        direction = -1;
        this->hasLastGlyph = false;
        this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
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
        // In RTL mode, cursor.x is the right boundary — subtract glyph width
        // before drawing so the glyph fits within the canvas.
        if (this->direction == -1) {
            this->cursor.x -= metrics.size.width * this->textSize;
        }
        this->lastGlyphPosition = this->cursor;
        int advance = drawGlyph(this->cursor.x, this->cursor.y, metrics, traits, glyph);
        if (this->direction == 1) {
            this->cursor.x += advance;
        }
    }

    return 1;
}

int CanvasView::drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph) {
    uint8_t width = glyphRect.size.width;
    uint8_t bytesPerRow = (width + 7) / 8;
    bool mirrored = (this->direction == -1) && traits.is.mirrored;
    int bbxOffset = glyphRect.origin.x;

    // .text emphasis: italic (depth 1 or 3) uses shear, bold (depth 2 or 3) draws twice
    bool bold = (this->emphasisDepth == 2 || this->emphasisDepth == 3);
    int shear = (this->emphasisDepth == 1 || this->emphasisDepth == 3)
                ? this->glyphRowCount / 4 : 0;

    for (int row = 0; row < this->glyphRowCount; row++) {
        int shift = (shear && this->glyphRowCount > 1)
                    ? shear * (this->glyphRowCount - 1 - row) / (this->glyphRowCount - 1) : 0;

        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            uint8_t line = glyph[row * bytesPerRow + byteIdx];
            int xOffset = byteIdx * 8;

            for (int8_t j = 7; j >= 0; j--, line >>= 1) {
                if (line & 1) {
                    int pixelX = mirrored ? (width - 1 - (xOffset + j)) : (xOffset + j);
                    if (this->textSize == 1) {
                        drawPixel(x + bbxOffset + pixelX + shift, y + row, this->textColor);
                        if (bold) drawPixel(x + bbxOffset + pixelX + shift + 1, y + row, this->textColor);
                    } else {
                        fillRect((x + bbxOffset + pixelX + shift) * this->textSize, y + row * this->textSize,
                                 this->textSize, this->textSize, this->textColor);
                        if (bold) fillRect((x + bbxOffset + pixelX + shift + 1) * this->textSize, y + row * this->textSize,
                                          this->textSize, this->textSize, this->textColor);
                    }
                }
            }
        }
    }

    return width * this->textSize;
}
