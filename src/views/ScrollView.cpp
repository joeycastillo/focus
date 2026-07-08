/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
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

#include "ScrollView.hpp"

namespace focus {

ScrollView::ScrollView(Rect rect) : View(rect) {
    this->setClipsToBounds(true);
}

void ScrollView::setFrame(Rect rect) {
    View::setFrame(rect);
    // A resized viewport can strand the offset past the end; re-clamp.
    this->setScrollOffset(this->getScrollOffset());
}

void ScrollView::setContentSize(Size size) {
    this->contentSize = size;
    this->setScrollOffset(this->getScrollOffset());
}

Size ScrollView::getContentSize() const {
    return this->contentSize;
}

Point ScrollView::clampOffset(Point offset) const {
    int maxX = this->contentSize.width - this->frame.size.width;
    int maxY = this->contentSize.height - this->frame.size.height;
    if (maxX < 0) maxX = 0;
    if (maxY < 0) maxY = 0;
    if (offset.x < 0) offset.x = 0;
    if (offset.y < 0) offset.y = 0;
    if (offset.x > maxX) offset.x = maxX;
    if (offset.y > maxY) offset.y = maxY;
    return offset;
}

void ScrollView::setScrollOffset(Point offset) {
    offset = this->clampOffset(offset);
    Rect bounds = this->getBounds();
    if (offset.x == bounds.origin.x && offset.y == bounds.origin.y) return;
    this->setBounds(MakeRect(offset.x, offset.y, bounds.size.width, bounds.size.height));
}

Point ScrollView::getScrollOffset() const {
    return this->getBounds().origin;
}

}  // namespace focus
