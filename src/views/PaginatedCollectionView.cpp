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

#include "PaginatedCollectionView.hpp"
#include "CollectionViewDataSource.hpp"
#include "CanvasView.hpp"
#include "Button.hpp"
#include "LabelView.hpp"
#include "Window.hpp"

PaginatedCollectionView::PaginatedCollectionView(Rect rect) : View(rect) {
    this->collectionView = std::make_shared<CollectionView>(MakeRect(0, 0, rect.size.width, rect.size.height));
    // Note: collectionView is added as a subview in rebuildLayout(), not here.
    // addSubview() calls shared_from_this(), which is invalid inside a constructor.
}

void PaginatedCollectionView::setDataSource(CollectionViewDataSource *dataSource, std::weak_ptr<void> owner) {
    this->collectionView->setDataSource(dataSource, owner);
}

void PaginatedCollectionView::setDelegate(CollectionViewDelegate *delegate, std::weak_ptr<void> owner) {
    this->collectionView->setDelegate(delegate, owner);
}

void PaginatedCollectionView::setLayout(CollectionViewLayout layout) {
    this->currentLayout = layout;
    this->collectionView->setLayout(layout);
}

void PaginatedCollectionView::setItemSize(Size size) {
    this->collectionView->setItemSize(size);
}

void PaginatedCollectionView::setPaginationStyle(PaginationStyle style) {
    this->paginationStyle = style;
    rebuildLayout();
}

void PaginatedCollectionView::reloadData() {
    this->collectionView->reloadData();
    updateIndicators();
}

void PaginatedCollectionView::goToNextPage() {
    size_t current = this->collectionView->getCurrentPage();
    size_t total = this->collectionView->getPageCount();
    if (current + 1 < total) {
        this->collectionView->goToPage(current + 1);
        updateIndicators();
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            window->setNeedsDisplay(true);
        }
    }
}

void PaginatedCollectionView::goToPreviousPage() {
    size_t current = this->collectionView->getCurrentPage();
    if (current > 0) {
        this->collectionView->goToPage(current - 1);
        updateIndicators();
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            window->setNeedsDisplay(true);
        }
    }
}

size_t PaginatedCollectionView::getCurrentPage() const {
    return this->collectionView->getCurrentPage();
}

size_t PaginatedCollectionView::getPageCount() const {
    return this->collectionView->getPageCount();
}

size_t PaginatedCollectionView::getItemsPerPage() const {
    return this->collectionView->getItemsPerPage();
}

void PaginatedCollectionView::rebuildLayout() {
    // Remove all existing subviews
    while (!this->subviews.empty()) {
        this->removeSubview(this->subviews.back());
    }
    this->beforeIndicator = nullptr;
    this->afterIndicator = nullptr;
    this->footerContainer = nullptr;
    this->prevButton = nullptr;
    this->pageLabel = nullptr;
    this->nextButton = nullptr;

    int w = this->frame.size.width;
    int h = this->frame.size.height;

    bool isVertical = (this->currentLayout != CollectionViewLayout::HorizontalList);

    switch (this->paginationStyle) {
        case PaginationStyle::None: {
            this->collectionView->setFrame(MakeRect(0, 0, w, h));
            this->addSubview(this->collectionView);
            break;
        }

        case PaginationStyle::Arrows: {
            if (isVertical) {
                // Top arrow strip, collection, bottom arrow strip
                this->beforeIndicator = std::make_shared<CanvasView>(
                    MakeRect(0, 0, w, kArrowThickness));
                int collectionH = h - 2 * kArrowThickness;
                this->collectionView->setFrame(
                    MakeRect(0, kArrowThickness, w, collectionH));
                this->afterIndicator = std::make_shared<CanvasView>(
                    MakeRect(0, h - kArrowThickness, w, kArrowThickness));
            } else {
                // Left arrow strip, collection, right arrow strip
                this->beforeIndicator = std::make_shared<CanvasView>(
                    MakeRect(0, 0, kArrowThickness, h));
                int collectionW = w - 2 * kArrowThickness;
                this->collectionView->setFrame(
                    MakeRect(kArrowThickness, 0, collectionW, h));
                this->afterIndicator = std::make_shared<CanvasView>(
                    MakeRect(w - kArrowThickness, 0, kArrowThickness, h));
            }

            drawArrow(this->beforeIndicator, false);
            drawArrow(this->afterIndicator, true);

            // Touch actions on the arrow indicators
            this->beforeIndicator->setAction(
                [this](Event, std::weak_ptr<View>) { this->goToPreviousPage(); },
                FOCUS_EVENT_TOUCH_UP_INSIDE);
            this->afterIndicator->setAction(
                [this](Event, std::weak_ptr<View>) { this->goToNextPage(); },
                FOCUS_EVENT_TOUCH_UP_INSIDE);

            this->addSubview(this->beforeIndicator);
            this->addSubview(this->collectionView);
            this->addSubview(this->afterIndicator);
            break;
        }

        case PaginationStyle::Footer: {
            // Collection view fills most of the space; footer strip at bottom
            int collectionH = h - kFooterGap - kFooterThickness;
            this->collectionView->setFrame(MakeRect(0, 0, w, collectionH));

            int footerY = collectionH + kFooterGap;
            this->footerContainer = std::make_shared<View>(
                MakeRect(0, footerY, w, kFooterThickness));
            this->footerContainer->setOpaque(false);

            int buttonWidth = 100;
            int labelWidth = w - 2 * buttonWidth - 2 * 8;

            this->prevButton = std::make_shared<Button>(
                MakeRect(0, 0, buttonWidth, kFooterThickness), "< Prev");
            this->prevButton->setAction(
                [this](Event, std::weak_ptr<View>) { this->goToPreviousPage(); },
                FOCUS_EVENT_TOUCH_UP_INSIDE);

            this->pageLabel = std::make_shared<LabelView>(
                MakeRect(buttonWidth + 8, 0, labelWidth, kFooterThickness), "");
            this->pageLabel->setTextAlignment(TextAlignmentCenter);

            this->nextButton = std::make_shared<Button>(
                MakeRect(w - buttonWidth, 0, buttonWidth, kFooterThickness), "Next >");
            this->nextButton->setAction(
                [this](Event, std::weak_ptr<View>) { this->goToNextPage(); },
                FOCUS_EVENT_TOUCH_UP_INSIDE);

            this->footerContainer->addSubview(this->prevButton);
            this->footerContainer->addSubview(this->pageLabel);
            this->footerContainer->addSubview(this->nextButton);

            this->addSubview(this->collectionView);
            this->addSubview(this->footerContainer);
            break;
        }
    }
}

void PaginatedCollectionView::updateIndicators() {
    size_t current = this->collectionView->getCurrentPage();
    size_t total = this->collectionView->getPageCount();

    switch (this->paginationStyle) {
        case PaginationStyle::None:
            break;

        case PaginationStyle::Arrows:
            if (this->beforeIndicator) {
                this->beforeIndicator->setHidden(total <= 1 || current == 0);
            }
            if (this->afterIndicator) {
                this->afterIndicator->setHidden(total <= 1 || current + 1 >= total);
            }
            break;

        case PaginationStyle::Footer:
            if (this->footerContainer) {
                this->footerContainer->setHidden(total <= 1);
            }
            if (this->prevButton) {
                this->prevButton->setEnabled(current > 0);
            }
            if (this->nextButton) {
                this->nextButton->setEnabled(current + 1 < total);
            }
            updateFooterLabel();
            break;
    }
}

void PaginatedCollectionView::drawArrow(std::shared_ptr<CanvasView> canvas, bool forward) {
    int cw = canvas->getCanvasWidth();
    int ch = canvas->getCanvasHeight();
    uint16_t black = GrayscaleColor::Black();

    canvas->clear(GrayscaleColor::White());

    bool isVertical = (this->currentLayout != CollectionViewLayout::HorizontalList);

    if (isVertical) {
        // Draw a triangle centered horizontally in the strip
        int arrowHeight = 8;
        int arrowBase = 16;
        int cx = cw / 2;
        int startY = (ch - arrowHeight) / 2;

        for (int row = 0; row < arrowHeight; row++) {
            int lineWidth;
            if (forward) {
                // Down arrow: wide at top, narrow at bottom
                lineWidth = arrowBase - (arrowBase * row / arrowHeight);
            } else {
                // Up arrow: narrow at top, wide at bottom
                lineWidth = arrowBase * (row + 1) / arrowHeight;
            }
            if (lineWidth < 1) lineWidth = 1;
            int x0 = cx - lineWidth / 2;
            canvas->fillRect(x0, startY + row, lineWidth, 1, black);
        }
    } else {
        // Draw a triangle centered vertically in the strip
        int arrowWidth = 8;
        int arrowBase = 16;
        int cy = ch / 2;
        int startX = (cw - arrowWidth) / 2;

        for (int col = 0; col < arrowWidth; col++) {
            int lineHeight;
            if (forward) {
                // Right arrow: tall at left, narrow at right
                lineHeight = arrowBase - (arrowBase * col / arrowWidth);
            } else {
                // Left arrow: narrow at left, tall at right
                lineHeight = arrowBase * (col + 1) / arrowWidth;
            }
            if (lineHeight < 1) lineHeight = 1;
            int y0 = cy - lineHeight / 2;
            canvas->fillRect(startX + col, y0, 1, lineHeight, black);
        }
    }
}

void PaginatedCollectionView::updateFooterLabel() {
    if (!this->pageLabel) return;
    size_t current = this->collectionView->getCurrentPage() + 1;
    size_t total = this->collectionView->getPageCount();
    if (total > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Page %zu of %zu", current, total);
        this->pageLabel->setText(buf);
    }
}
