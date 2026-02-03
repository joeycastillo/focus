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

#include "CanvasView.hpp"
#include "Display.hpp"
#include <algorithm>

CanvasView::CanvasView(Rect rect)
    : View(rect),
      buffer(rect.size.width * rect.size.height, 255) {
    this->opaque = true;
}

void CanvasView::draw(int x, int y) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        int screenX = x + this->frame.origin.x;
        int screenY = y + this->frame.origin.y;
        int w = this->frame.size.width;
        int h = this->frame.size.height;
        int blackColor = display->getBlackColor();
        int whiteColor = display->getWhiteColor();

        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                uint8_t val = buffer[py * w + px];
                display->drawPixel(screenX + px, screenY + py,
                                   val == 0 ? blackColor : whiteColor);
            }
        }
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
    buffer[y * frame.size.width + x] = (color == 0) ? 0 : 255;
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
    uint8_t val = (color == 0) ? 0 : 255;
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= frame.size.height) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= frame.size.width) continue;
            buffer[j * frame.size.width + i] = val;
        }
    }
}

void CanvasView::clear(int color) {
    uint8_t val = (color == 0) ? 0 : 255;
    std::fill(buffer.begin(), buffer.end(), val);
}
