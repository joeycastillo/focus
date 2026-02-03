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

#pragma once

#include "View.hpp"
#include <vector>

/// A view that owns its own pixel buffer for programmatic drawing.
/// Content is drawn to the internal buffer via drawPixel/drawRect/fillRect,
/// then blitted to the Display during the normal view draw cycle.
class CanvasView : public View {
public:
    CanvasView(Rect rect);

    void draw(int x, int y) override;

    // Drawing API — coordinates are local to the canvas (0,0 = top-left)
    void drawPixel(int x, int y, int color);
    void drawRect(int x, int y, int w, int h, int color);
    void fillRect(int x, int y, int w, int h, int color);
    void clear(int color);

    int getBlackColor() { return 0; }
    int getWhiteColor() { return 1; }

    int getCanvasWidth() { return frame.size.width; }
    int getCanvasHeight() { return frame.size.height; }

private:
    std::vector<uint8_t> buffer;  // 1 byte per pixel: 0 = black, 255 = white
};
