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

#include "Focus.hpp"

class Display {
public:
    virtual void drawPixel(int x, int y, int color) = 0;

    // virtual void drawLine(int x1, int y1, int x2, int y2, int color) = 0;

    virtual void drawRect(int x, int y, int w, int h, int color) = 0;
    virtual void fillRect(int x, int y, int w, int h, int color) = 0;
    // virtual void drawRoundRect(int x, int y, int w, int h, int r, int color);
    // virtual void fillRoundRect(int x, int y, int w, int h, int r, int color);
    // virtual void drawCircle(int x, int y, int r, int color);
    // virtual void fillCircle(int x, int y, int r, int color);
    // virtual void drawEllipse(int x, int y, int rx, int ry, int color);
    // virtual void fillEllipse(int x, int y, int rx, int ry, int color);

    // virtual void fillScreen(int color);

    int drawText(int x, int y, int width, int height, int color, const char * utf8String);

    virtual int getBlackColor() = 0;
    virtual int getWhiteColor() = 0;

    virtual ~Display() {}
protected:
};
