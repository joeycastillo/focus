/*
 * MIT License
 *
 * Copyright (c) 2022-2026 Joey Castillo
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

void Display::blitOpaque(int x, int y, int w, int h,
                         const uint8_t* data, int rowBytes,
                         Rect clipRect) {
    if (this->displayMode == DisplayMode::Grayscale) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                uint8_t byte = data[row * rowBytes + col];
                uint16_t color = (uint16_t)byte << 8 | byte;
                this->fillRect(x + col, y + row, 1, 1, color, clipRect);
            }
        }
    } else {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int byteIdx = col >> 3;
                int bitIdx = 7 - (col & 7);
                bool set = (data[row * rowBytes + byteIdx] >> bitIdx) & 1;
                uint16_t color = set ? 0xFFFF : 0x0000;
                this->fillRect(x + col, y + row, 1, 1, color, clipRect);
            }
        }
    }
}
