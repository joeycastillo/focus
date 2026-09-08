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

#include "StackView.hpp"
#include <cstddef>

namespace focus {

StackView::StackView(Rect rect, Axis axis) : View(rect), axis(axis) {
    if (axis == Axis::Vertical) {
        this->setDirectionalAffinity(DirectionalAffinity::Vertical);
    } else {
        this->setDirectionalAffinity(DirectionalAffinity::Horizontal);
    }
}

void StackView::setSpacing(int spacing) {
    this->spacing = spacing;
    this->layoutSubviews();
}

int StackView::getSpacing() const {
    return this->spacing;
}

void StackView::setMargins(int top, int right, int bottom, int left) {
    this->marginTop = top;
    this->marginRight = right;
    this->marginBottom = bottom;
    this->marginLeft = left;
    this->layoutSubviews();
}

void StackView::setMargins(int uniform) {
    this->setMargins(uniform, uniform, uniform, uniform);
}

void StackView::addSubview(std::shared_ptr<View> view) {
    this->insertSubview(view, this->subviews.size());
}

void StackView::insertSubview(std::shared_ptr<View> view, size_t index) {
    // Re-inserting an existing child: drop it first so the index lands after removal.
    if (view->getSuperview() == this) {
        this->removeSubview(view);
    }
    if (index > this->subviews.size()) index = this->subviews.size();
    bool vertical = (this->axis == Axis::Vertical);
    int preferredSize = vertical ? view->getFrame().size.height
                                 : view->getFrame().size.width;
    this->preferredSizes.insert(this->preferredSizes.begin() + static_cast<std::ptrdiff_t>(index),
                                preferredSize);
    View::insertSubview(view, index);
    this->layoutSubviews();
}

void StackView::removeSubview(std::shared_ptr<View> view) {
    for (size_t i = 0; i < this->subviews.size(); i++) {
        if (this->subviews[i] == view) {
            this->preferredSizes.erase(this->preferredSizes.begin() + i);
            break;
        }
    }
    View::removeSubview(view);
    this->layoutSubviews();
}

void StackView::setFrame(Rect rect) {
    View::setFrame(rect);
    this->layoutSubviews();
}

void StackView::layoutSubviews() {
    size_t count = this->subviews.size();
    if (count == 0) return;
    if (this->preferredSizes.size() != count) return;

    bool vertical = (this->axis == Axis::Vertical);
    int leadingMargin = vertical ? this->marginTop : this->marginLeft;
    int trailingMargin = vertical ? this->marginBottom : this->marginRight;
    int crossLeading = vertical ? this->marginLeft : this->marginTop;
    int crossTrailing = vertical ? this->marginRight : this->marginBottom;
    int stackSize = (vertical ? this->frame.size.height : this->frame.size.width)
                    - leadingMargin - trailingMargin;
    int crossSize = (vertical ? this->frame.size.width : this->frame.size.height)
                    - crossLeading - crossTrailing;

    // Pass 1: sum fixed sizes and count flexible children.
    int fixedTotal = 0;
    int flexCount = 0;
    for (size_t i = 0; i < count; i++) {
        int childSize = this->preferredSizes[i];
        if (childSize > 0) {
            fixedTotal += childSize;
        } else {
            flexCount++;
        }
    }

    int totalSpacing = (count > 1) ? (int)(count - 1) * this->spacing : 0;
    int remaining = stackSize - fixedTotal - totalSpacing;
    if (remaining < 0) remaining = 0;

    int flexSize = (flexCount > 0) ? remaining / flexCount : 0;
    int flexRemainder = (flexCount > 0) ? remaining % flexCount : 0;

    // Pass 2: position each child.
    int offset = leadingMargin;
    int flexIndex = 0;
    for (size_t i = 0; i < count; i++) {
        int childSize = this->preferredSizes[i];
        if (childSize <= 0) {
            // Flexible child: equal share, last one absorbs remainder.
            flexIndex++;
            childSize = flexSize;
            if (flexIndex == flexCount) {
                childSize += flexRemainder;
            }
        }

        Rect childFrame;
        if (vertical) {
            childFrame = MakeRect(crossLeading, offset, crossSize, childSize);
        } else {
            childFrame = MakeRect(offset, crossLeading, childSize, crossSize);
        }
        this->subviews[i]->setFrame(childFrame);

        offset += childSize + this->spacing;
    }
}

}  // namespace focus
