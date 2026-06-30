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

#include "CollectionView.hpp"

namespace focus {

CollectionView::CollectionView(Rect rect) : View(rect) {
}

void CollectionView::setDataSource(CollectionViewDataSource* dataSource, std::weak_ptr<void> owner) {
    this->dataSource = dataSource;
    if (owner.lock()) {
        this->dataSourceOwner = owner;
    } else {
        this->dataSourceOwner = std::nullopt;
    }
}

void CollectionView::setDelegate(CollectionViewDelegate* delegate, std::weak_ptr<void> owner) {
    this->delegate = delegate;
    if (owner.lock()) {
        this->delegateOwner = owner;
    } else {
        this->delegateOwner = std::nullopt;
    }
}

void CollectionView::setLayout(CollectionViewLayout layout) {
    this->layout = layout;
    if (layout == CollectionViewLayout::HorizontalList) {
        this->setDirectionalAffinity(DirectionalAffinity::Horizontal);
    } else if (layout == CollectionViewLayout::Grid) {
        this->setDirectionalAffinity(DirectionalAffinity::None);
    } else {
        this->setDirectionalAffinity(DirectionalAffinity::Vertical);
    }
}

void CollectionView::setItemSize(Size size) {
    this->itemSize = size;
}

void CollectionView::setItemSpacing(int spacing) {
    this->itemSpacing = spacing;
}

AccessibilityRole CollectionView::accessibilityRole() const {
    return AccessibilityRole::List;
}

}  // namespace focus
