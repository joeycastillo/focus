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

#include "Button.hpp"
#include "Window.hpp"
#include "Display.hpp"

Button::Button(Rect rect, std::string text) : Control(rect) {
    this->text = text;
}

void Button::draw(int x, int y) {
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        View::draw(x, y);
        if (std::shared_ptr<Display> display = this->getWindow().lock()->getDisplay().lock()) {
            int textHeight = 16;  // default fallback
            if (auto glyphProvider = display->getDefaultGlyphProvider()) {
                textHeight = glyphProvider->getGlyphRowCount();
            }
            int verticalOffset = (this->frame.size.height - textHeight) / 2;
            Rect layoutRect = MakeRect(this->frame.origin.x + x, this->frame.origin.y + y + verticalOffset, this->frame.size.width, textHeight);
            if (this->focused) {
                display->fillRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                display->drawText(layoutRect, this->backgroundColor, 1, this->text.c_str());
            } else {
                display->drawRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                display->drawText(layoutRect, this->foregroundColor, 1, this->text.c_str());
            }
        }
    }
}
