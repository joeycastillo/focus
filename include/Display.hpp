/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
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

#pragma once

#include <stdint.h>

class Display {
public:
    virtual void fillRect(int x, int y, int w, int h, int color) = 0;

    /// Blit a 1bpp MSB-first buffer to the display, overwriting all pixels in the region.
    /// White bits (1) write white; black bits (0) write black.
    virtual void blitOpaque(int x, int y, int w, int h,
                            const uint8_t* data, int rowBytes) = 0;

    /// Write a solid color to the display only where mask bits are set (1).
    /// Pixels where mask bit is 0 are left unchanged.
    virtual void blitMasked(int x, int y, int w, int h, int color,
                            const uint8_t* mask, int rowBytes) = 0;

    virtual int getBlackColor() = 0;
    virtual int getWhiteColor() = 0;

    virtual void forceFullRefresh() {}

    virtual ~Display() {}
};
