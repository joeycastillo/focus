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

extern const uint16_t _unicode_info_0000_33FF[];

CanvasView::CanvasView(Rect rect)
    : View(rect),
      rowBytes((rect.size.width > 0 && rect.size.height > 0) ? (rect.size.width + 7) / 8 : 0),
      planeSize((rect.size.width > 0 && rect.size.height > 0) ? ((rect.size.width + 7) / 8) * rect.size.height : 0),
      buffer((rect.size.width > 0 && rect.size.height > 0) ? ((rect.size.width + 7) / 8) * rect.size.height : 0, 0xFF) {
    this->opaque = true;
}

void CanvasView::setFrame(Rect rect) {
    if (rect.size.width != this->frame.size.width || rect.size.height != this->frame.size.height) {
        int w = rect.size.width;
        int h = rect.size.height;
        if (w > 0 && h > 0) {
            if (this->canvasMode == DisplayMode::Grayscale) {
                this->rowBytes = w;
                this->planeSize = w * h;
            } else if (this->canvasMode == DisplayMode::RGB565) {
                this->rowBytes = w * 2;
                this->planeSize = w * 2 * h;
            } else {
                this->rowBytes = (w + 7) / 8;
                this->planeSize = this->rowBytes * h;
            }
            this->buffer.assign(this->planeSize, 0xFF);
        } else {
            this->rowBytes = 0;
            this->planeSize = 0;
            this->buffer.clear();
        }
    }
    View::setFrame(rect);
}

void CanvasView::setCanvasMode(DisplayMode mode) {
    canvasMode = mode;
    if (mode == DisplayMode::Grayscale) {
        rowBytes = frame.size.width;
        buffer.resize(frame.size.width * frame.size.height, 0xFF);
    } else if (mode == DisplayMode::RGB565) {
        rowBytes = frame.size.width * 2;
        buffer.resize(frame.size.width * frame.size.height * 2);
        buffer.shrink_to_fit();
    } else {
        rowBytes = (frame.size.width + 7) / 8;
        planeSize = rowBytes * frame.size.height;
        buffer.resize(planeSize);
        buffer.shrink_to_fit();
    }
}

void CanvasView::setCanvasRotation(int degrees) {
    this->canvasRotation = (degrees / 90) & 0x03;
}

void CanvasView::mapToBuffer(int x, int y, int &bx, int &by) const {
    switch (this->canvasRotation) {
        case 1:  // 90° CW
            bx = this->frame.size.width - 1 - y;
            by = x;
            break;
        case 2:  // 180°
            bx = this->getCanvasWidth() - 1 - x;
            by = this->getCanvasHeight() - 1 - y;
            break;
        case 3:  // 270° CW
            bx = y;
            by = this->frame.size.height - 1 - x;
            break;
        default: // 0°
            bx = x;
            by = y;
            break;
    }
}

void CanvasView::drawContent(int x, int y, Rect clipRect) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                            this->frame.size.width, this->frame.size.height,
                            this->buffer.data(), this->rowBytes, clipRect);
    }
}

void CanvasView::drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= this->getCanvasWidth() || y < 0 || y >= this->getCanvasHeight()) return;
    int bx, by;
    this->mapToBuffer(x, y, bx, by);

    if (canvasMode == DisplayMode::Grayscale) {
        buffer[by * frame.size.width + bx] = (uint8_t)(color >> 8);
    } else if (canvasMode == DisplayMode::RGB565) {
        reinterpret_cast<uint16_t*>(buffer.data())[by * frame.size.width + bx] = color;
    } else {
        int idx = by * rowBytes + (bx >> 3);
        uint8_t mask = 0x80 >> (bx & 7);
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
    // Clamp to logical canvas bounds
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(this->getCanvasWidth(), x + w);
    int y1 = std::min(this->getCanvasHeight(), y + h);
    if (x0 >= x1 || y0 >= y1) return;

    if (this->canvasRotation != 0) {
        // Rotated: per-pixel fallback (logical horizontal spans become
        // non-contiguous in the physical buffer for 90°/270°).
        // TODO: for large rotated canvases, consider writing a rotation-aware
        // bulk fill that maps logical rows to physical columns directly instead
        // of paying the mapToBuffer + bounds-check overhead per pixel.
        for (int fy = y0; fy < y1; fy++) {
            for (int fx = x0; fx < x1; fx++) {
                this->drawPixel(fx, fy, color);
            }
        }
        return;
    }

    if (canvasMode == DisplayMode::Grayscale) {
        uint8_t val = (uint8_t)(color >> 8);
        int width = frame.size.width;
        for (int row = y0; row < y1; row++) {
            std::memset(&buffer[row * width + x0], val, x1 - x0);
        }
    } else if (canvasMode == DisplayMode::RGB565) {
        int width = frame.size.width;
        uint16_t* pixels = reinterpret_cast<uint16_t*>(buffer.data());
        int spanW = x1 - x0;
        if ((color >> 8) == (color & 0xFF)) {
            uint8_t byte = color & 0xFF;
            for (int row = y0; row < y1; row++) {
                std::memset(&buffer[row * rowBytes + x0 * 2], byte, spanW * 2);
            }
        } else {
            uint16_t* firstRow = pixels + y0 * width + x0;
            for (int col = 0; col < spanW; col++) firstRow[col] = color;
            for (int row = y0 + 1; row < y1; row++) {
                std::memcpy(pixels + row * width + x0, firstRow, spanW * 2);
            }
        }
    } else {
        _fillPlane(buffer.data(), x0, y0, x1, y1, (color != 0) ? 0xFF : 0x00);
    }
}

void CanvasView::invertRect(int x, int y, int w, int h) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(this->getCanvasWidth(), x + w);
    int y1 = std::min(this->getCanvasHeight(), y + h);
    if (x0 >= x1 || y0 >= y1) return;

    if (canvasMode == DisplayMode::Grayscale) {
        int width = frame.size.width;
        if (this->canvasRotation != 0) {
            for (int iy = y0; iy < y1; iy++) {
                for (int ix = x0; ix < x1; ix++) {
                    int bx, by;
                    this->mapToBuffer(ix, iy, bx, by);
                    buffer[by * width + bx] ^= 0xFF;
                }
            }
        } else {
            for (int row = y0; row < y1; row++) {
                for (int col = x0; col < x1; col++) {
                    buffer[row * width + col] ^= 0xFF;
                }
            }
        }
        return;
    }

    if (canvasMode == DisplayMode::RGB565) {
        int width = frame.size.width;
        uint16_t* pixels = reinterpret_cast<uint16_t*>(buffer.data());
        if (this->canvasRotation != 0) {
            for (int iy = y0; iy < y1; iy++) {
                for (int ix = x0; ix < x1; ix++) {
                    int bx, by;
                    this->mapToBuffer(ix, iy, bx, by);
                    pixels[by * width + bx] ^= 0xFFFF;
                }
            }
        } else {
            for (int row = y0; row < y1; row++) {
                for (int col = x0; col < x1; col++) {
                    pixels[row * width + col] ^= 0xFFFF;
                }
            }
        }
        return;
    }

    if (this->canvasRotation != 0) {
        // Rotated: per-pixel fallback using XOR on buffer coordinates.
        for (int iy = y0; iy < y1; iy++) {
            for (int ix = x0; ix < x1; ix++) {
                int bx, by;
                this->mapToBuffer(ix, iy, bx, by);
                int idx = by * rowBytes + (bx >> 3);
                uint8_t mask = 0x80 >> (bx & 7);
                buffer[idx] ^= mask;
            }
        }
        return;
    }

    int firstByte = x0 >> 3;
    int lastByte = (x1 - 1) >> 3;
    int startBit = x0 & 7;
    int endBit = (x1 - 1) & 7;

    for (int row = y0; row < y1; row++) {
        int rowOffset = row * rowBytes;
        if (firstByte == lastByte) {
            uint8_t mask = (0xFF >> startBit) & (0xFF << (7 - endBit));
            buffer[rowOffset + firstByte] ^= mask;
        } else {
            if (startBit > 0) {
                buffer[rowOffset + firstByte] ^= (0xFF >> startBit);
            }
            int midStart = firstByte + (startBit > 0 ? 1 : 0);
            for (int b = midStart; b < lastByte; b++) {
                buffer[rowOffset + b] ^= 0xFF;
            }
            buffer[rowOffset + lastByte] ^= (0xFF << (7 - endBit));
        }
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
    if (canvasMode == DisplayMode::Grayscale) {
        std::memset(buffer.data(), (uint8_t)(color >> 8), buffer.size());
    } else if (canvasMode == DisplayMode::RGB565) {
        if ((color >> 8) == (color & 0xFF)) {
            std::memset(buffer.data(), color & 0xFF, buffer.size());
        } else {
            int total = frame.size.width * frame.size.height;
            uint16_t* pixels = reinterpret_cast<uint16_t*>(buffer.data());
            for (int i = 0; i < total; i++) pixels[i] = color;
        }
    } else {
        std::memset(buffer.data(), (color != 0) ? 0xFF : 0x00, planeSize);
    }
}

void CanvasView::drawMask(int x, int y, int w, int h,
                          const uint8_t* mask, int maskRowBytes, uint16_t color) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int maskByte = row * maskRowBytes + (col >> 3);
            uint8_t maskBit = 0x80 >> (col & 7);
            if (mask[maskByte] & maskBit) {
                this->drawPixel(x + col, y + row, color);
            }
        }
    }
}

void CanvasView::drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void CanvasView::applyCheckerboardMask(uint16_t color) {
    if (canvasMode == DisplayMode::Grayscale) {
        uint8_t val = (uint8_t)(color >> 8);
        int width = frame.size.width;
        for (int y = 0; y < frame.size.height; y++) {
            for (int x = 0; x < width; x++) {
                if ((x + y) & 1) {
                    buffer[y * width + x] = val;
                }
            }
        }
    } else if (canvasMode == DisplayMode::RGB565) {
        uint16_t* pixels = reinterpret_cast<uint16_t*>(buffer.data());
        int width = frame.size.width;
        for (int y = 0; y < frame.size.height; y++) {
            for (int x = 0; x < width; x++) {
                if ((x + y) & 1) pixels[y * width + x] = color;
            }
        }
    } else {
        auto applyToPlane = [&](uint8_t* plane, bool setBits) {
            for (int y = 0; y < frame.size.height; y++) {
                uint8_t pattern = (y & 1) ? 0x55 : 0xAA;
                for (int b = 0; b < rowBytes; b++) {
                    if (setBits) {
                        plane[y * rowBytes + b] |= pattern;
                    } else {
                        plane[y * rowBytes + b] &= ~pattern;
                    }
                }
            }
        };
        applyToPlane(buffer.data(), color != 0);
    }
}

// --- Font and text rendering ---

void CanvasView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
}

void CanvasView::setWordMapOutput(std::vector<WordPosition> *output) {
    this->wordMapOutput = output;
}


int CanvasView::drawText(Rect layoutRect, uint16_t color, int text_size, const char *utf8String,
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
    if (text_size < 1) text_size = 1;
    if (text_size > 16) text_size = 16;

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
    this->emphasisDepth = 0;
    this->lastWasNewline = false;

    UNICODE_CODEPOINT *codepoints = (UNICODE_CODEPOINT *)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (!codepoints) return 0;

    utf8_parse((char *)utf8String, codepoints);

    // Build byte offset map for word position tracking (before shaping,
    // so offsets correspond to the original UTF-8 string).
    if (this->wordMapOutput) {
        this->wordMapOutput->clear();
        uint32_t *offsets = (uint32_t *)malloc((len + 1) * sizeof(uint32_t));
        if (offsets) {
            uint32_t bytePos = 0;
            for (size_t i = 0; i < len; i++) {
                offsets[i] = bytePos;
                bytePos += TextLayout::bytesForCodepoint(codepoints[i]);
            }
            offsets[len] = bytePos;
        }
        this->codepointByteOffsets = offsets;
    }

    // Auto-detect Arabic codepoints (U+0621–U+06D2) and shape if present
    bool needsShaping = false;
    for (size_t i = 0; i < len; i++) {
        if (codepoints[i] >= 0x0621 && codepoints[i] <= 0x06D2) {
            needsShaping = true;
            break;
        }
    }
    if (needsShaping) {
        shapeArabic(codepoints, len);
    }
    size_t retVal = this->writeCodepoints(codepoints, len, glyphProvider);

    if (this->codepointByteOffsets) {
        free(this->codepointByteOffsets);
        this->codepointByteOffsets = nullptr;
    }
    free(codepoints);

    return retVal;
}

void CanvasView::renderBidiLine(UNICODE_CODEPOINT *codepoints, size_t lineStart, size_t lineLen,
                                int paragraphDir, int16_t effectiveWidth, int16_t indentedOriginX,
                                GlyphProvider *glyphProvider) {
    if (lineLen == 0) return;

    // Set initial cursor.x based on paragraph direction
    if (paragraphDir == 1) {
        this->cursor.x = indentedOriginX;
    } else {
        this->cursor.x = indentedOriginX + effectiveWidth;
    }

    // Justification state: extra pixels to add after each word gap (space).
    // Computed in the justified branch below, consumed in emitCodepoint.
    int16_t justifyExtraPerGap = 0;
    int16_t justifyRemainder = 0;
    int16_t justifyGapsEmitted = 0;
    int16_t justifyMaxGaps = 0;

    // Apply text alignment offset for this line
    if (this->textAlignment != TextAlignment::Left) {
        int16_t lineWidth = measureCodepointsWidth(codepoints + lineStart, lineLen, glyphProvider);
        int16_t slack = effectiveWidth - lineWidth;
        if (slack > 0) {
            if (this->textAlignment == TextAlignment::Center) {
                if (paragraphDir == 1) {
                    this->cursor.x = indentedOriginX + slack / 2;
                } else {
                    this->cursor.x = indentedOriginX + effectiveWidth - slack / 2;
                }
            } else if (this->textAlignment == TextAlignment::Right) {
                if (paragraphDir == 1) {
                    this->cursor.x = indentedOriginX + slack;
                }
                // RTL right-align is the default (cursor at right edge)
            } else if (this->textAlignment == TextAlignment::Justified) {
                // Count word gaps (space codepoints)
                int16_t numSpaces = 0;
                for (size_t k = 0; k < lineLen; k++) {
                    if (codepoints[lineStart + k] == 0x20) numSpaces++;
                }
                // Word-wrapped lines include a trailing space at the break
                // point. Exclude it from justification: it's not a visible
                // inter-word gap, and its width should become part of the
                // slack distributed across the real gaps.
                bool hasTrailingSpace = (lineLen > 0 && codepoints[lineStart + lineLen - 1] == 0x20);
                int16_t interWordGaps = hasTrailingSpace ? numSpaces - 1 : numSpaces;
                if (interWordGaps > 0) {
                    int16_t adjustedSlack = slack;
                    if (hasTrailingSpace) {
                        adjustedSlack += measureCodepointsWidth(
                            &codepoints[lineStart + lineLen - 1], 1, glyphProvider);
                    }
                    justifyMaxGaps = interWordGaps;
                    justifyExtraPerGap = adjustedSlack / interWordGaps;
                    justifyRemainder = adjustedSlack % interWordGaps;
                }
                // Cursor stays at left edge — justified starts flush left
            }
        }
    }

    // Simplified UAX#9 bidi algorithm: resolve each codepoint in this line
    // to a directional run (L or R), then render runs in visual order.
    this->bidiResolveBuffer.resize(lineLen);
    uint8_t *resolved = this->bidiResolveBuffer.data();
    for (size_t i = 0; i < lineLen; i++) {
        resolved[i] = getTraitsForCodepoint(codepoints[lineStart + i]).is.bidi_class;
    }

    // Step 2: Resolve weak types (simplified W rules)
    for (size_t i = 0; i < lineLen; i++) {
        uint8_t bc = resolved[i];
        if (bc == BIDI_NSM) {
            resolved[i] = (i > 0) ? resolved[i - 1] : (uint8_t)(paragraphDir == -1 ? BIDI_R : BIDI_L);
        }
    }
    for (size_t i = 0; i < lineLen; i++) {
        if (resolved[i] == BIDI_AL) resolved[i] = BIDI_R;
    }
    for (size_t i = 1; i + 1 < lineLen; i++) {
        if (resolved[i] == BIDI_ES && resolved[i-1] == BIDI_EN && resolved[i+1] == BIDI_EN) {
            resolved[i] = BIDI_EN;
        } else if (resolved[i] == BIDI_CS) {
            if (resolved[i-1] == BIDI_EN && resolved[i+1] == BIDI_EN) resolved[i] = BIDI_EN;
            else if (resolved[i-1] == BIDI_AN && resolved[i+1] == BIDI_AN) resolved[i] = BIDI_AN;
        }
    }
    for (size_t i = 0; i < lineLen; i++) {
        if (resolved[i] == BIDI_ET) {
            bool adjacentEN = false;
            if (i > 0 && resolved[i-1] == BIDI_EN) adjacentEN = true;
            if (i + 1 < lineLen && resolved[i+1] == BIDI_EN) adjacentEN = true;
            if (adjacentEN) resolved[i] = BIDI_EN;
        }
    }
    for (size_t i = 0; i < lineLen; i++) {
        if (resolved[i] == BIDI_ES || resolved[i] == BIDI_ET || resolved[i] == BIDI_CS) {
            resolved[i] = BIDI_ON;
        }
    }
    for (size_t i = 0; i < lineLen; i++) {
        if (resolved[i] == BIDI_EN) {
            for (int j = (int)i - 1; j >= 0; j--) {
                if (resolved[j] == BIDI_L) { resolved[i] = BIDI_L; break; }
                if (resolved[j] == BIDI_R) break;
            }
            if (resolved[i] == BIDI_EN && paragraphDir == 1) resolved[i] = BIDI_L;
        }
    }

    // Step 3: Resolve neutrals (simplified N rules)
    for (size_t i = 0; i < lineLen; i++) {
        uint8_t bc = resolved[i];
        if (bc == BIDI_ON || bc == BIDI_WS || bc == BIDI_BN ||
            bc == BIDI_B || bc == BIDI_S) {
            uint8_t prevStrong = paragraphDir == -1 ? BIDI_R : BIDI_L;
            for (int j = (int)i - 1; j >= 0; j--) {
                if (resolved[j] == BIDI_L || resolved[j] == BIDI_R ||
                    resolved[j] == BIDI_AN || resolved[j] == BIDI_EN) {
                    prevStrong = (resolved[j] == BIDI_L) ? BIDI_L : BIDI_R;
                    break;
                }
            }
            uint8_t nextStrong = paragraphDir == -1 ? BIDI_R : BIDI_L;
            for (size_t j = i + 1; j < lineLen; j++) {
                if (resolved[j] == BIDI_L || resolved[j] == BIDI_R ||
                    resolved[j] == BIDI_AN || resolved[j] == BIDI_EN) {
                    nextStrong = (resolved[j] == BIDI_L) ? BIDI_L : BIDI_R;
                    break;
                }
            }
            if (prevStrong == nextStrong) {
                resolved[i] = prevStrong;
            } else {
                resolved[i] = paragraphDir == -1 ? BIDI_R : BIDI_L;
            }
        } else if (bc == BIDI_EN || bc == BIDI_AN) {
            resolved[i] = (bc == BIDI_AN) ? BIDI_R : BIDI_L;
        }
    }

    // Word tracking state for this line
    bool trackingWord = false;
    int16_t wordMinX = 0, wordMaxX = 0, wordY = 0;
    uint32_t wordStartOffset = 0, wordEndOffset = 0;
    int16_t wordLineHeight = TextLayout::getLineHeight(glyphProvider, this->textSize, this->lineSpacing);

    auto emitCodepoint = [&](size_t j) {
        UNICODE_CODEPOINT cp = codepoints[lineStart + j];
        int16_t beforeX = this->cursor.x;
        this->writeCodepoint(cp, glyphProvider);
        // Justified alignment: distribute extra space after each inter-word gap
        if (justifyMaxGaps > 0 && cp == 0x20 && justifyGapsEmitted < justifyMaxGaps) {
            int16_t extra = justifyExtraPerGap + (justifyGapsEmitted < justifyRemainder ? 1 : 0);
            this->cursor.x += extra * this->direction;
            justifyGapsEmitted++;
        }
        if (this->wordMapOutput && this->codepointByteOffsets) {
            int16_t afterX = this->cursor.x;
            unicode_info_t traits = getTraitsForCodepoint(cp);
            uint8_t wb = traits.is.word_break;

            // Word characters: always start or continue a word
            bool isWordChar = (wb == WB_ALetter || wb == WB_Hebrew_Letter ||
                               wb == WB_Numeric || wb == WB_Katakana ||
                               wb == WB_ExtendNumLet || wb == WB_Extend);

            // Mid-word characters: continue a word only if followed by a word character
            bool isMidWord = (wb == WB_MidLetter || wb == WB_MidNum ||
                              wb == WB_MidNumLet || wb == WB_Single_Quote);

            if (isWordChar) {
                int16_t left = std::min(beforeX, afterX);
                int16_t right = std::max(beforeX, afterX);
                if (!trackingWord) {
                    wordMinX = left;
                    wordMaxX = right;
                    wordY = this->cursor.y;
                    wordStartOffset = this->codepointByteOffsets[lineStart + j];
                    trackingWord = true;
                } else {
                    wordMinX = std::min(wordMinX, left);
                    wordMaxX = std::max(wordMaxX, right);
                }
                wordEndOffset = this->codepointByteOffsets[lineStart + j + 1];
            } else if (trackingWord && isMidWord && (j + 1) < lineLen) {
                // Lookahead: if the next codepoint is a word character, include
                // this mid-word character (handles contractions like "don't"
                // and decimals like "3.14").
                unicode_info_t nextTraits = getTraitsForCodepoint(codepoints[lineStart + j + 1]);
                uint8_t nextWb = nextTraits.is.word_break;
                bool nextIsWord = (nextWb == WB_ALetter || nextWb == WB_Hebrew_Letter ||
                                   nextWb == WB_Numeric || nextWb == WB_Katakana ||
                                   nextWb == WB_ExtendNumLet || nextWb == WB_Extend);
                if (nextIsWord) {
                    int16_t left = std::min(beforeX, afterX);
                    int16_t right = std::max(beforeX, afterX);
                    wordMinX = std::min(wordMinX, left);
                    wordMaxX = std::max(wordMaxX, right);
                    wordEndOffset = this->codepointByteOffsets[lineStart + j + 1];
                } else {
                    // Trailing punctuation — emit the word before it
                    this->wordMapOutput->push_back({wordMinX, wordY,
                        (int16_t)(wordMaxX - wordMinX), wordLineHeight,
                        wordStartOffset, wordEndOffset});
                    trackingWord = false;
                }
            } else {
                // Boundary character — emit any tracked word
                if (trackingWord) {
                    this->wordMapOutput->push_back({wordMinX, wordY,
                        (int16_t)(wordMaxX - wordMinX), wordLineHeight,
                        wordStartOffset, wordEndOffset});
                    trackingWord = false;
                }
            }
        }
    };

    // Step 4: Build and render directional runs
    size_t i = 0;
    while (i < lineLen) {
        bool runIsRTL = (resolved[i] == BIDI_R);
        size_t runStart = i;
        while (i < lineLen && (resolved[i] == BIDI_R) == runIsRTL) {
            i++;
        }
        size_t runLen = i - runStart;
        int16_t runWidth = measureCodepointsWidth(
            codepoints + lineStart + runStart, runLen, glyphProvider);

        if (paragraphDir == -1) {
            this->cursor.x -= runWidth;
            int16_t runLeftEdge = this->cursor.x;

            if (runIsRTL) {
                this->direction = -1;
                this->hasLastGlyph = false;
                int16_t rtlCursor = runLeftEdge + runWidth;
                this->cursor.x = rtlCursor;
                for (size_t j = runStart; j < runStart + runLen; j++) {
                    emitCodepoint(j);
                }
            } else {
                this->direction = 1;
                this->hasLastGlyph = false;
                this->cursor.x = runLeftEdge;
                for (size_t j = runStart; j < runStart + runLen; j++) {
                    emitCodepoint(j);
                }
            }
            this->cursor.x = runLeftEdge;
            this->hasLastGlyph = false;
        } else {
            if (runIsRTL) {
                this->direction = -1;
                this->hasLastGlyph = false;
                int16_t runRightEdge = this->cursor.x + runWidth;
                this->cursor.x = runRightEdge;
                for (size_t j = runStart; j < runStart + runLen; j++) {
                    emitCodepoint(j);
                }
                this->cursor.x = runRightEdge;
            } else {
                this->direction = 1;
                this->hasLastGlyph = false;
                for (size_t j = runStart; j < runStart + runLen; j++) {
                    emitCodepoint(j);
                }
            }
        }
    }

    // Close any word still open at end of this line
    if (trackingWord && this->wordMapOutput) {
        this->wordMapOutput->push_back({wordMinX, wordY,
            (int16_t)(wordMaxX - wordMinX), wordLineHeight,
            wordStartOffset, wordEndOffset});
    }
}

int16_t CanvasView::measureCodepointsWidth(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    int16_t width = 0;
    int16_t lastAdvance = 0;
    uint8_t emphasis = (uint8_t)this->emphasisDepth;
    const GlyphMetrics* asciiMetrics = glyphProvider->getAsciiMetricsCache();
    bool emphasisAware = emphasis > 0 || glyphProvider->supportsEmphasis(1)
                                      || glyphProvider->supportsEmphasis(2);
    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];
        if (cp == 0x08) {
            width -= lastAdvance;
            if (width < 0) width = 0;
            lastAdvance = 0;
            continue;
        }
        if (cp == 0x0E) { emphasis = emphasis < 3 ? emphasis + 1 : 3; continue; }
        if (cp == 0x0F) { emphasis = emphasis > 0 ? emphasis - 1 : 0; continue; }
        if (cp < 0x20) continue;
        unicode_info_t traits;
        GlyphMetrics metrics;
        if (cp < 0x80 && (!emphasisAware || emphasis == 0)) {
            traits.packed = _unicode_info_0000_33FF[cp];
            metrics = asciiMetrics[cp - 0x20];
        } else {
            if (cp < 0x80) {
                traits.packed = _unicode_info_0000_33FF[cp];
            } else {
                traits = getTraitsForCodepoint(cp);
            }
            metrics = glyphProvider->metricsForCodepoint(cp, emphasis);
        }
        if (!(traits.is.nsm || traits.is.controlchar)) {
            int16_t advance = metrics.advance * this->textSize;
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

    // Pre-detect paragraph direction from first strongly-directional character
    int paragraphDir = 1; // default LTR
    for (size_t i = 0; i < len; i++) {
        uint8_t bc = getTraitsForCodepoint(codepoints[i]).is.bidi_class;
        if (bidiIsRTL(bc)) {
            paragraphDir = -1;
            this->direction = -1;
            this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
            break;
        } else if (bidiIsLTR(bc)) {
            break;
        }
    }

    while (pos < len) {
        int16_t effectiveWidth = this->textLayoutRect.size.width;
        int16_t indentedOriginX = this->textLayoutRect.origin.x;

        WordWrapResult result = TextLayout::measureLineWrap(
            codepoints + pos,
            len - pos,
            effectiveWidth,
            this->textSize,
            glyphProvider,
            0,
            (uint8_t)this->emphasisDepth
        );

        int32_t numGlyphsToDraw;
        if (result.codepointsConsumed < 0) {
            numGlyphsToDraw = (int32_t)(len - pos);
        } else {
            numGlyphsToDraw = result.codepointsConsumed;
        }

        renderBidiLine(codepoints, pos, numGlyphsToDraw, paragraphDir,
                       effectiveWidth, indentedOriginX, glyphProvider);
        pos += numGlyphsToDraw;

        if (result.wrapped) {
            this->cursor.y += TextLayout::getLineHeight(glyphProvider, this->textSize, this->lineSpacing);
            if (this->direction == 1) {
                this->cursor.x = indentedOriginX;
            } else {
                this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
            }
        }

        // Also handle paragraph breaks (newlines)
        if (result.isParagraphBreak) {
            if (this->direction == 1) {
                this->cursor.x = this->textLayoutRect.origin.x;
            } else {
                this->cursor.x = this->textLayoutRect.origin.x + this->textLayoutRect.size.width;
            }
        }

        if (this->cursor.y >= (this->textLayoutRect.origin.y + this->textLayoutRect.size.height)) break;
    }

    return retVal;
}

size_t CanvasView::writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider) {
    if (codepoint == '\r') return 1; // Ignore CR; LF handles line breaks

    if (codepoint == '\n') {
        if (this->lastWasNewline) {
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

    // SO (Shift Out) increases emphasis depth
    if (codepoint == 0x0E) {
        this->emphasisDepth = std::min(this->emphasisDepth + 1, 3);
        return 1;
    }
    // SI (Shift In) decreases emphasis depth
    if (codepoint == 0x0F) {
        this->emphasisDepth = std::max(this->emphasisDepth - 1, 0);
        return 1;
    }
    // BS (0x08) — backspace for typewriter overprinting
    // Move cursor back to the last glyph position so the next character overprints
    if (codepoint == 0x08) {
        if (this->hasLastGlyph) {
            this->cursor.x = this->lastGlyphPosition.x;
        }
        // Detect _BS<char> underline pattern: if the previous glyph was _,
        // the next visible character needs its underscore redrawn to match width.
        this->pendingOverprintUnderline = this->lastGlyphWasUnderscore;
        this->lastGlyphWasUnderscore = false;
        return 1;
    }

    if (codepoint < 0x20) return 1;

    unicode_info_t traits = getTraitsForCodepoint(codepoint);
    if (traits.is.controlchar) return 1;

    this->lastWasNewline = false; // Visible character breaks consecutive newline tracking

    uint8_t emphasis = (uint8_t)this->emphasisDepth;
    GlyphMetrics metrics = glyphProvider->metricsForCodepoint(codepoint, emphasis);

    // Direction is set by the run-based renderer in writeCodepoints;
    // writeCodepoint just renders in the current direction.

    const uint8_t *glyph = glyphProvider->glyphForCodepoint(codepoint, emphasis);
    if (traits.is.nsm && this->hasLastGlyph) {
        drawGlyph(this->lastGlyphPosition.x, this->lastGlyphPosition.y, metrics, traits, glyph);
    } else {
        this->hasLastGlyph = true;
        // In RTL mode, cursor.x is the right boundary — subtract glyph width
        // before drawing so the glyph fits within the canvas.
        if (this->direction == -1) {
            this->cursor.x -= metrics.advance * this->textSize;
        }
        this->lastGlyphPosition = this->cursor;
        int advance = drawGlyph(this->cursor.x, this->cursor.y, metrics, traits, glyph);
        if (this->direction == 1) {
            this->cursor.x += advance;
        }
    }

    // Overprint underline: after drawing the character in a _BS<char> triplet,
    // redraw the underscore glyph to match the character's width. Draw once
    // left-aligned, and if the character is wider than the underscore, draw
    // again right-aligned. Covers characters up to 2x the underscore width.
    if (this->pendingOverprintUnderline) {
        GlyphMetrics uMetrics = glyphProvider->metricsForCodepoint('_');
        const uint8_t *uGlyph = glyphProvider->glyphForCodepoint('_');
        unicode_info_t uTraits = {};
        int16_t charWidth = metrics.advance;
        int16_t uWidth = uMetrics.advance;
        drawGlyph(this->lastGlyphPosition.x, this->lastGlyphPosition.y, uMetrics, uTraits, uGlyph);
        if (charWidth > uWidth) {
            int16_t rightAlignedX = this->lastGlyphPosition.x + (charWidth - uWidth);
            drawGlyph(rightAlignedX, this->lastGlyphPosition.y, uMetrics, uTraits, uGlyph);
        }
        this->pendingOverprintUnderline = false;
    }

    this->lastGlyphWasUnderscore = (codepoint == '_');

    return 1;
}

int CanvasView::drawGlyph(int16_t x, int16_t y, GlyphMetrics glyphRect, unicode_info_t traits, const uint8_t *glyph) {
    uint8_t renderWidth = glyphRect.bitmapWidth > glyphRect.advance ? glyphRect.bitmapWidth : glyphRect.advance;
    uint8_t bytesPerRow = (renderWidth + 7) / 8;
    bool mirrored = (this->direction == -1) && traits.is.mirrored;
    int bbxOffset = glyphRect.xOffset;
    int bbyOffset = glyphRect.yOffset;
    int drawRowCount = glyphRect.height > this->glyphRowCount ? glyphRect.height : this->glyphRowCount;

    // Synthetic effects are needed only for emphasis components the provider lacks.
    // The provider's smart fallback handles glyph selection (e.g., BI→I when BI is missing).
    const GlyphProvider *provider = this->font ? this->font->getGlyphProvider() : nullptr;
    bool bold = (this->emphasisDepth == 2 || this->emphasisDepth == 3)
                && (!provider || !provider->supportsEmphasis(2));
    int shear = ((this->emphasisDepth == 1 || this->emphasisDepth == 3)
                && (!provider || !provider->supportsEmphasis(1)))
                ? drawRowCount / 4 : 0;

    for (int row = 0; row < drawRowCount; row++) {
        int shift = (shear && drawRowCount > 1)
                    ? shear * (drawRowCount - 1 - row) / (drawRowCount - 1) : 0;

        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            uint8_t line = glyph[row * bytesPerRow + byteIdx];
            int xOffset = byteIdx * 8;

            for (int8_t j = 7; j >= 0; j--, line >>= 1) {
                if (line & 1) {
                    int pixelX = mirrored ? (glyphRect.bitmapWidth - 1 - (xOffset + j)) : (xOffset + j);
                    if (this->textSize == 1) {
                        drawPixel(x + bbxOffset + pixelX + shift, y + bbyOffset + row, this->textColor);
                        if (bold) drawPixel(x + bbxOffset + pixelX + shift + 1, y + bbyOffset + row, this->textColor);
                    } else {
                        fillRect((x + bbxOffset + pixelX + shift) * this->textSize, (y + bbyOffset + row) * this->textSize,
                                 this->textSize, this->textSize, this->textColor);
                        if (bold) fillRect((x + bbxOffset + pixelX + shift + 1) * this->textSize, (y + bbyOffset + row) * this->textSize,
                                          this->textSize, this->textSize, this->textColor);
                    }
                }
            }
        }
    }

    return glyphRect.advance * this->textSize;
}
