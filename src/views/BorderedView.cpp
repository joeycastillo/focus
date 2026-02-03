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

#include "BorderedView.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"

BorderedView::BorderedView(Rect rect) : View(rect) {
    this->opaque = true;
}

void BorderedView::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }
    this->canvas->clear(this->backgroundColor);
    this->canvas->drawRect(0, 0, this->frame.size.width, this->frame.size.height, this->foregroundColor);
    this->canvasValid = true;
}

void BorderedView::draw(int x, int y) {
    if (!this->canvasValid) this->renderCanvas();
    if (this->canvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->canvas->getBufferData(), this->canvas->getRowBytes());
        }
    }
    // Draw subviews on top
    int subviewX = x + this->frame.origin.x - this->bounds.origin.x;
    int subviewY = y + this->frame.origin.y - this->bounds.origin.y;
    for (std::shared_ptr<View> view : this->subviews) {
        if (!view->isHidden()) view->draw(subviewX, subviewY);
    }
}
