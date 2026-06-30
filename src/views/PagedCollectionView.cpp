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

#include "PagedCollectionView.hpp"
#include "CollectionViewDataSource.hpp"
#include "CollectionViewDelegate.hpp"
#include "CollectionViewCell.hpp"
#include "Window.hpp"

namespace focus {

PagedCollectionView::PagedCollectionView(Rect rect) : CollectionView(rect) {
}

void PagedCollectionView::setFrame(Rect rect) {
    Size oldSize = this->frame.size;
    View::setFrame(rect);
    if (this->dataLoaded &&
        (rect.size.width != oldSize.width || rect.size.height != oldSize.height)) {
        this->removeCurrentPageViews();
        if (this->variableItemSizes) {
            this->computePageBoundaries();
        }
        this->loadPage(this->currentPage);
    }
}

size_t PagedCollectionView::calculateItemsPerPage() const {
    if (this->variableItemSizes) {
        if (this->pageBoundaries.empty()) return 0;
        size_t page = this->currentPage;
        if (page >= this->pageBoundaries.size()) page = this->pageBoundaries.size() - 1;
        size_t start = this->pageBoundaries[page];
        size_t end;
        if (page + 1 < this->pageBoundaries.size()) {
            end = this->pageBoundaries[page + 1];
        } else {
            end = this->dataSource ? this->dataSource->numberOfItems(this) : start;
        }
        return end - start;
    }

    int s = this->itemSpacing;
    switch (this->layout) {
        case CollectionViewLayout::VerticalList:
            if (this->itemSize.height <= 0) return 0;
            return (this->frame.size.height + s) / (this->itemSize.height + s);
        case CollectionViewLayout::HorizontalList:
            if (this->itemSize.width <= 0) return 0;
            return (this->frame.size.width + s) / (this->itemSize.width + s);
        case CollectionViewLayout::Grid: {
            if (this->itemSize.width <= 0 || this->itemSize.height <= 0) return 0;
            int columns = (this->frame.size.width + s) / (this->itemSize.width + s);
            int rows = (this->frame.size.height + s) / (this->itemSize.height + s);
            return columns * rows;
        }
    }
    return 0;
}

size_t PagedCollectionView::getItemsPerPage() const {
    return this->calculateItemsPerPage();
}

size_t PagedCollectionView::getCurrentPage() const {
    return this->currentPage;
}

size_t PagedCollectionView::getPageCount() const {
    if (!this->dataSource) return 0;
    if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) return 0;

    if (this->variableItemSizes) {
        return this->pageBoundaries.size();
    }

    size_t itemsPerPage = this->calculateItemsPerPage();
    if (itemsPerPage == 0) return 0;
    size_t totalItems = this->dataSource->numberOfItems(this);
    return (totalItems + itemsPerPage - 1) / itemsPerPage;
}

void PagedCollectionView::computePageBoundaries() {
    this->pageBoundaries.clear();
    if (!this->dataSource) return;
    if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) return;

    size_t totalItems = this->dataSource->numberOfItems(this);
    if (totalItems == 0) return;

    int capacity = (this->layout == CollectionViewLayout::VerticalList)
        ? this->frame.size.height
        : this->frame.size.width;
    if (capacity <= 0) return;

    this->pageBoundaries.push_back(0);
    int accumulated = 0;
    bool pageEmpty = true;

    for (size_t i = 0; i < totalItems; i++) {
        Size size = this->dataSource->sizeForItemAtIndex(this, i);
        int dimension = (this->layout == CollectionViewLayout::VerticalList)
            ? size.height : size.width;
        int needed = pageEmpty ? dimension : this->itemSpacing + dimension;

        if (accumulated + needed > capacity && !pageEmpty) {
            this->pageBoundaries.push_back(i);
            accumulated = dimension;
            pageEmpty = false;
        } else {
            accumulated += needed;
            pageEmpty = false;
        }
    }
}

void PagedCollectionView::removeCurrentPageViews() {
    while (!this->subviews.empty()) {
        this->removeSubview(this->subviews.back());
    }
}

void PagedCollectionView::loadPage(size_t page) {
    if (!this->dataSource) return;
    if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) return;

    size_t totalItems = this->dataSource->numberOfItems(this);

    size_t startIndex, endIndex;
    if (this->variableItemSizes) {
        if (page >= this->pageBoundaries.size()) return;
        startIndex = this->pageBoundaries[page];
        endIndex = (page + 1 < this->pageBoundaries.size())
            ? this->pageBoundaries[page + 1] : totalItems;
    } else {
        size_t itemsPerPage = this->calculateItemsPerPage();
        if (itemsPerPage == 0) return;
        startIndex = page * itemsPerPage;
        if (startIndex >= totalItems) return;
        endIndex = startIndex + itemsPerPage;
        if (endIndex > totalItems) endIndex = totalItems;
    }

    int s = this->itemSpacing;
    int columns = 1;
    if (this->layout == CollectionViewLayout::Grid) {
        columns = (this->frame.size.width + s) / (this->itemSize.width + s);
        if (columns < 1) columns = 1;
    }

    // Check if we have a live delegate for automatic cell action wiring
    CollectionViewDelegate* liveDelegate = nullptr;
    if (this->delegate) {
        if (!this->delegateOwner.has_value() || !this->delegateOwner->expired()) {
            liveDelegate = this->delegate;
        }
    }

    int runningOffset = 0;

    for (size_t i = startIndex; i < endIndex; i++) {
        size_t indexInPage = i - startIndex;
        Rect itemFrame;

        if (this->variableItemSizes) {
            Size size = this->dataSource->sizeForItemAtIndex(this, i);
            if (this->layout == CollectionViewLayout::VerticalList) {
                itemFrame = MakeRect(0, runningOffset, this->frame.size.width, size.height);
                runningOffset += size.height + s;
            } else {
                itemFrame = MakeRect(runningOffset, 0, size.width, this->frame.size.height);
                runningOffset += size.width + s;
            }
        } else {
            int itemX = 0;
            int itemY = 0;
            int itemW = this->itemSize.width;
            int itemH = this->itemSize.height;

            switch (this->layout) {
                case CollectionViewLayout::VerticalList:
                    itemY = (int)indexInPage * (this->itemSize.height + s);
                    itemW = this->frame.size.width;
                    break;
                case CollectionViewLayout::HorizontalList:
                    itemX = (int)indexInPage * (this->itemSize.width + s);
                    itemH = this->frame.size.height;
                    break;
                case CollectionViewLayout::Grid: {
                    int col = (int)(indexInPage % columns);
                    int row = (int)(indexInPage / columns);
                    itemX = col * (this->itemSize.width + s);
                    itemY = row * (this->itemSize.height + s);
                    break;
                }
            }

            itemFrame = MakeRect(itemX, itemY, itemW, itemH);
        }

        auto cell = this->dataSource->cellForItemAtIndex(this, i, itemFrame);
        if (cell) {
            if (liveDelegate) {
                size_t globalIndex = i;
                CollectionView* cv = this;
                // Capture the delegate owner weak_ptr so lambdas can verify
                // the delegate is still alive before invoking it.
                std::weak_ptr<void> ownerWeak = this->delegateOwner.value_or(std::weak_ptr<void>{});
                bool hasOwner = this->delegateOwner.has_value();
                cell->setAction(
                    [liveDelegate, cv, globalIndex, ownerWeak, hasOwner](Event, std::weak_ptr<View>) {
                        if (hasOwner && ownerWeak.expired()) return;
                        liveDelegate->didSelectItemAtIndex(cv, globalIndex);
                    },
                    FOCUS_EVENT_TOUCH_UP_INSIDE);
                cell->setAction(
                    [liveDelegate, cv, globalIndex, ownerWeak, hasOwner](Event, std::weak_ptr<View>) {
                        if (hasOwner && ownerWeak.expired()) return;
                        liveDelegate->didLongPressItemAtIndex(cv, globalIndex);
                    },
                    FOCUS_EVENT_LONG_PRESS);
                cell->onFocusChanged = [liveDelegate, cv, globalIndex, ownerWeak, hasOwner](CollectionViewCell& c, bool focused) {
                    if (hasOwner && ownerWeak.expired()) return;
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

bool PagedCollectionView::handleEvent(Event event) {
    if (this->layout == CollectionViewLayout::Grid) {
        switch (event.type) {
            case FOCUS_EVENT_DIRECTION_LEFT:
            case FOCUS_EVENT_DIRECTION_RIGHT:
            case FOCUS_EVENT_DIRECTION_UP:
            case FOCUS_EVENT_DIRECTION_DOWN:
            {
                auto window = this->getWindow().lock();
                if (!window) return false;
                auto focusedView = window->getFocusedView().lock();
                if (!focusedView) return false;

                int index = this->indexOfChildContaining(focusedView);
                if (index < 0) return false;
                if (this->itemSize.width <= 0) return View::handleEvent(event);

                int columns = (this->frame.size.width + this->itemSpacing) / (this->itemSize.width + this->itemSpacing);
                if (columns < 1) columns = 1;
                int count = (int)this->subviews.size();
                int target = -1;

                switch (event.type) {
                    case FOCUS_EVENT_DIRECTION_LEFT:
                        if (index % columns != 0)
                            target = index - 1;
                        break;
                    case FOCUS_EVENT_DIRECTION_RIGHT:
                        if ((index + 1) % columns != 0 && index + 1 < count)
                            target = index + 1;
                        break;
                    case FOCUS_EVENT_DIRECTION_UP:
                        if (index >= columns)
                            target = index - columns;
                        break;
                    case FOCUS_EVENT_DIRECTION_DOWN:
                        if (index + columns < count)
                            target = index + columns;
                        break;
                    default:
                        break;
                }

                if (target >= 0 && target < count) {
                    this->subviews[target]->becomeFocused();
                    return true;
                }
                // At edge — bubble to parent for pagination, cross-container nav, etc.
                if (this->getSuperview()) {
                    return this->getSuperview()->handleEvent(event);
                }
                return false;
            }
            default:
                break;
        }
    }
    return View::handleEvent(event);
}

void PagedCollectionView::reloadData() {
    this->removeCurrentPageViews();
    this->currentPage = 0;

    // Detect variable-size mode: for non-grid layouts, check if the data source
    // provides a custom size for the first item.
    this->variableItemSizes = false;
    this->pageBoundaries.clear();
    if (this->dataSource && this->layout != CollectionViewLayout::Grid) {
        if (this->dataSourceOwner.has_value() && this->dataSourceOwner->expired()) {
            // Data source expired, skip
        } else if (this->dataSource->numberOfItems(this) > 0) {
            Size probe = this->dataSource->sizeForItemAtIndex(this, 0);
            if (probe.width != 0 || probe.height != 0) {
                this->variableItemSizes = true;
                this->computePageBoundaries();
            }
        }
    }

    this->dataLoaded = true;
    this->loadPage(0);

    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void PagedCollectionView::goToPage(size_t page) {
    size_t pageCount = this->getPageCount();
    if (pageCount == 0) return;
    if (page >= pageCount) page = pageCount - 1;

    // F2: if we are already on this page and it has been built, navigating to it
    // again would tear down and rebuild identical cells. The clamp above runs
    // first so goToPage(out-of-range) still no-ops when it resolves to the
    // current page. This guard lives on PagedCollectionView (not the
    // PaginatedCollectionView wrapper) so the wrapper's updateIndicators() still
    // runs after a no-op — footer label and arrow visibility stay correct. The
    // only same-page caller (CollectionViewController::viewDidLayoutSubviews ->
    // goToPage(savedPageIndex)) always runs reloadData() first, which already
    // built and invalidated this page, so the skipped rebuild is redundant.
    if (page == this->currentPage && this->dataLoaded) {
        return;
    }

    this->removeCurrentPageViews();
    this->currentPage = page;
    this->loadPage(page);

    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

}  // namespace focus
