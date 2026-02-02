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

#include "BitmapView.hpp"
#include "Window.hpp"
#include "Display.hpp"

BitmapView::BitmapView(Rect rect, const unsigned char *bitmap) : View(rect) {
    this->bitmap = bitmap;
}

void BitmapView::draw(int x, int y) {
    View::draw(x, y);
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        // const uint8_t *pSprite, int cx, int cy, int iPitch, int x, int y, uint8_t iColor
        int dx = 0;
        int dy = 0;
        for(int i = 0; i < this->frame.size.width * this->frame.size.height / 8; i++) {
            unsigned char b = this->bitmap[i];
            unsigned char mask = 0x80;
            while(mask) {
                if (!!(b & mask)) display->drawPixel(this->frame.origin.x + dx, this->frame.origin.y + dy, this->foregroundColor);
                dx++;
                if (dx >= this->frame.size.width) {
                    dx = 0;
                    dy++;
                }
                mask >>= 1;
            }
        }
    }
}
