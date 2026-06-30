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

#include "HatchedView.hpp"
#include "Window.hpp"
#include "Display.hpp"

namespace focus {

HatchedView::HatchedView(Rect rect, uint16_t color) : View(rect) {
    this->foregroundColor = color;
    this->opaque = false;

    // Generate hatching mask: bit set where (x + y) % 4 != 0
    this->maskRowBytes = (rect.size.width + 7) / 8;
    this->mask.resize(this->maskRowBytes * rect.size.height, 0);
    for (int my = 0; my < rect.size.height; my++) {
        for (int mx = 0; mx < rect.size.width; mx++) {
            if ((mx + my) % 4) {
                this->mask[my * this->maskRowBytes + (mx >> 3)] |= (0x80 >> (mx & 7));
            }
        }
    }
}

void HatchedView::setFrame(Rect rect) {
    if (rect.size.width != this->frame.size.width || rect.size.height != this->frame.size.height) {
        this->maskRowBytes = (rect.size.width + 7) / 8;
        this->mask.assign(this->maskRowBytes * rect.size.height, 0);
        for (int my = 0; my < rect.size.height; my++) {
            for (int mx = 0; mx < rect.size.width; mx++) {
                if ((mx + my) % 4) {
                    this->mask[my * this->maskRowBytes + (mx >> 3)] |= (0x80 >> (mx & 7));
                }
            }
        }
    }
    View::setFrame(rect);
}

void HatchedView::drawContent(int x, int y, Rect clipRect) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        display->blitMasked(x + this->frame.origin.x, y + this->frame.origin.y,
                            this->frame.size.width, this->frame.size.height,
                            this->foregroundColor, this->mask.data(), this->maskRowBytes, clipRect);
    }
}

}  // namespace focus
