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

#include "LabelView.hpp"
#include "Window.hpp"
#include "Display.hpp"

LabelView::LabelView(Rect rect, std::string text) : View(rect) {
    this->text = text;
}

void LabelView::draw(int x, int y) {
    View::draw(x, y);
    if (std::shared_ptr<Display> display = this->getWindow().lock()->getDisplay().lock()) {
        display->drawText(this->frame.origin.x + x, this->frame.origin.y + y, this->frame.size.width, this->frame.size.height, this->foregroundColor, this->textScale, this->text.c_str());
    }
}

void LabelView::setText(std::string text) {
    this->text = text;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void LabelView::setTextScale(uint8_t scale) {
    this->textScale = scale;
}