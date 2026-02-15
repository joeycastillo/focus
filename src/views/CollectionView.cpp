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
#include "CollectionViewDataSource.hpp"
#include "CollectionViewDelegate.hpp"
#include "CollectionViewCell.hpp"
#include "Window.hpp"

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
        this->setDirectionalAffinity(DirectionalAffinityHorizontal);
    } else {
        this->setDirectionalAffinity(DirectionalAffinityVertical);
    }
}

void CollectionView::setItemSize(Size size) {
    this->itemSize = size;
}

size_t CollectionView::calculateItemsPerPage() const {
    if (this->itemSize.width <= 0 || this->itemSize.height <= 0) return 0;

    switch (this->layout) {
        case CollectionViewLayout::VerticalList:
            return this->frame.size.height / this->itemSize.height;
        case CollectionViewLayout::HorizontalList:
            return this->frame.size.width / this->itemSize.width;
        case CollectionViewLayout::Grid: {
            int columns = this->frame.size.width / this->itemSize.width;
            int rows = this->frame.size.height / this->itemSize.height;
            return columns * rows;
        }
    }
    return 0;
}

size_t CollectionView::getItemsPerPage() const {
    return this->calculateItemsPerPage();
}

size_t CollectionView::getCurrentPage() const {
    return this->currentPage;
}

size_t CollectionView::getPageCount() const {
    if (!this->dataSource) return 0;
    if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) return 0;
    size_t itemsPerPage = this->calculateItemsPerPage();
    if (itemsPerPage == 0) return 0;
    size_t totalItems = this->dataSource->numberOfItems(const_cast<CollectionView*>(this));
    return (totalItems + itemsPerPage - 1) / itemsPerPage;
}

void CollectionView::removeCurrentPageViews() {
    while (!this->subviews.empty()) {
        this->removeSubview(this->subviews.back());
    }
}

void CollectionView::loadPage(size_t page) {
    if (!this->dataSource) return;
    if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) return;

    size_t itemsPerPage = this->calculateItemsPerPage();
    if (itemsPerPage == 0) return;

    size_t totalItems = this->dataSource->numberOfItems(this);
    size_t startIndex = page * itemsPerPage;
    if (startIndex >= totalItems) return;

    size_t endIndex = startIndex + itemsPerPage;
    if (endIndex > totalItems) endIndex = totalItems;

    int columns = 1;
    if (this->layout == CollectionViewLayout::Grid) {
        columns = this->frame.size.width / this->itemSize.width;
        if (columns < 1) columns = 1;
    }

    // Check if we have a live delegate for automatic cell action wiring
    CollectionViewDelegate* liveDelegate = nullptr;
    if (this->delegate) {
        if (!this->delegateOwner.has_value() || !this->delegateOwner->expired()) {
            liveDelegate = this->delegate;
        }
    }

    for (size_t i = startIndex; i < endIndex; i++) {
        size_t indexInPage = i - startIndex;
        int itemX = 0;
        int itemY = 0;

        switch (this->layout) {
            case CollectionViewLayout::VerticalList:
                itemY = (int)indexInPage * this->itemSize.height;
                break;
            case CollectionViewLayout::HorizontalList:
                itemX = (int)indexInPage * this->itemSize.width;
                break;
            case CollectionViewLayout::Grid: {
                int col = (int)(indexInPage % columns);
                int row = (int)(indexInPage / columns);
                itemX = col * this->itemSize.width;
                itemY = row * this->itemSize.height;
                break;
            }
        }

        Rect itemFrame = MakeRect(itemX, itemY, this->itemSize.width, this->itemSize.height);
        auto cell = this->dataSource->cellForItemAtIndex(this, i, itemFrame);
        if (cell) {
            if (liveDelegate) {
                size_t globalIndex = i;
                CollectionView* cv = this;
                cell->setAction(
                    [liveDelegate, cv, globalIndex](Event, std::weak_ptr<View>) {
                        liveDelegate->didSelectItemAtIndex(cv, globalIndex);
                    },
                    FOCUS_EVENT_TOUCH_UP_INSIDE);
                cell->onFocusChanged = [liveDelegate, cv, globalIndex](CollectionViewCell& c, bool focused) {
                    if (focused) {
                        liveDelegate->didFocusItemAtIndex(cv, globalIndex, c);
                    } else {
                        liveDelegate->didUnfocusItemAtIndex(cv, globalIndex, c);
                    }
                };
            }
            this->addSubview(cell);
        }
    }
}

void CollectionView::reloadData() {
    this->removeCurrentPageViews();
    this->currentPage = 0;
    this->loadPage(0);
}

void CollectionView::goToPage(size_t page) {
    size_t pageCount = this->getPageCount();
    if (pageCount == 0) return;
    if (page >= pageCount) page = pageCount - 1;

    this->removeCurrentPageViews();
    this->currentPage = page;
    this->loadPage(page);

    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}
