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
      rowBytes((rect.size.width + 7) / 8),
      planeSize(((rect.size.width + 7) / 8) * rect.size.height),
      buffer(((rect.size.width + 7) / 8) * rect.size.height, 0xFF) {
    this->opaque = true;
}

void CanvasView::setCanvasMode(DisplayMode mode) {
    canvasMode = mode;
    if (mode == DisplayMode::TwoBpp) {
        buffer.resize(2 * planeSize, 0xFF);
    } else {
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
        if (canvasMode == DisplayMode::TwoBpp) {
            display->blitOpaque2bpp(x + this->frame.origin.x, y + this->frame.origin.y,
                                    this->frame.size.width, this->frame.size.height,
                                    this->buffer.data(), this->rowBytes, clipRect);
        } else {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->buffer.data(), this->rowBytes, clipRect);
        }
    }
}

void CanvasView::drawPixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= this->getCanvasWidth() || y < 0 || y >= this->getCanvasHeight()) return;
    int bx, by;
    this->mapToBuffer(x, y, bx, by);
    int idx = by * rowBytes + (bx >> 3);
    uint8_t mask = 0x80 >> (bx & 7);

    if (canvasMode == DisplayMode::TwoBpp) {
        // plane0 (buffer) stores bit 1 (high bit) of color
        if (color & 0x02) {
            buffer[idx] |= mask;
        } else {
            buffer[idx] &= ~mask;
        }
        // plane1 stores bit 0 (low bit) of color
        if (color & 0x01) {
            buffer[planeSize + idx] |= mask;
        } else {
            buffer[planeSize + idx] &= ~mask;
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
    // Clamp to logical canvas bounds
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(this->getCanvasWidth(), x + w);
    int y1 = std::min(this->getCanvasHeight(), y + h);
    if (x0 >= x1 || y0 >= y1) return;

    if (this->canvasRotation != 0) {
        // Rotated: per-pixel fallback (logical horizontal spans become
        // non-contiguous in the physical buffer for 90°/270°).
        for (int fy = y0; fy < y1; fy++) {
            for (int fx = x0; fx < x1; fx++) {
                this->drawPixel(fx, fy, color);
            }
        }
        return;
    }

    if (canvasMode == DisplayMode::TwoBpp) {
        _fillPlane(buffer.data(), x0, y0, x1, y1, (color & 0x02) ? 0xFF : 0x00);
        _fillPlane(buffer.data() + planeSize, x0, y0, x1, y1, (color & 0x01) ? 0xFF : 0x00);
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
    if (canvasMode == DisplayMode::TwoBpp) {
        std::memset(buffer.data(), (color & 0x02) ? 0xFF : 0x00, planeSize);
        std::memset(buffer.data() + planeSize, (color & 0x01) ? 0xFF : 0x00, planeSize);
    } else {
        std::memset(buffer.data(), (color != 0) ? 0xFF : 0x00, planeSize);
    }
}

void CanvasView::applyCheckerboardMask(uint16_t color) {
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

    if (canvasMode == DisplayMode::TwoBpp) {
        applyToPlane(buffer.data(), (color & 0x02) != 0);
        applyToPlane(buffer.data() + planeSize, (color & 0x01) != 0);
    } else {
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

    // Apply text alignment offset for this line
    if (this->textAlignment != TextAlignmentLeft) {
        int16_t lineWidth = measureCodepointsWidth(codepoints + lineStart, lineLen, glyphProvider);
        int16_t slack = effectiveWidth - lineWidth;
        if (slack > 0) {
            if (this->textAlignment == TextAlignmentCenter) {
                if (paragraphDir == 1) {
                    this->cursor.x = indentedOriginX + slack / 2;
                } else {
                    this->cursor.x = indentedOriginX + effectiveWidth - slack / 2;
                }
            } else if (this->textAlignment == TextAlignmentRight) {
                if (paragraphDir == 1) {
                    this->cursor.x = indentedOriginX + slack;
                }
                // RTL right-align is the default (cursor at right edge)
            }
        }
    }

    // Simplified UAX#9 bidi algorithm: resolve each codepoint in this line
    // to a directional run (L or R), then render runs in visual order.
    uint8_t resolved[lineLen];
    for (size_t i = 0; i < lineLen; i++) {
        resolved[i] = getTraitsForCodepoint(codepoints[lineStart + i]).is.bidi_class;
    }

    // Step 2: Resolve weak types (simplified W rules)
    for (size_t i = 0; i < lineLen; i++) {
        uint8_t bc = resolved[i];
        if (bc == BIDI_NSM) {
            resolved[i] = (i > 0) ? resolved[i - 1] : (paragraphDir == -1 ? BIDI_R : BIDI_L);
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
            glyphProvider
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

    Rect metrics = glyphProvider->metricsForCodepoint(codepoint);

    // Direction is set by the run-based renderer in writeCodepoints;
    // writeCodepoint just renders in the current direction.

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

    // Overprint underline: after drawing the character in a _BS<char> triplet,
    // redraw the underscore glyph to match the character's width. Draw once
    // left-aligned, and if the character is wider than the underscore, draw
    // again right-aligned. Covers characters up to 2x the underscore width.
    if (this->pendingOverprintUnderline) {
        Rect uMetrics = glyphProvider->metricsForCodepoint('_');
        uint8_t *uGlyph = glyphProvider->glyphForCodepoint('_');
        unicode_info_t uTraits = {};
        int16_t charWidth = metrics.size.width;
        int16_t uWidth = uMetrics.size.width;
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
