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

#include "CollectionViewController.hpp"
#include "Application.hpp"
#include "Window.hpp"

CollectionViewController::CollectionViewController(std::shared_ptr<Application> application)
    : ViewController(application) {
}

void CollectionViewController::setItemSize(Size size) {
    this->configuredItemSize = size;
    if (this->paginatedView) {
        this->paginatedView->setItemSize(size);
    }
}

void CollectionViewController::setLayout(CollectionViewLayout layout) {
    this->configuredLayout = layout;
    if (this->paginatedView) {
        this->paginatedView->setLayout(layout);
    }
}

void CollectionViewController::setPaginationStyle(PaginationStyle style) {
    this->configuredPaginationStyle = style;
    if (this->paginatedView) {
        this->paginatedView->setPaginationStyle(style);
    }
}

void CollectionViewController::reloadData() {
    if (this->paginatedView) {
        this->paginatedView->reloadData();
    }
}

void CollectionViewController::createView() {
    ViewController::createView();

    auto app = this->application.lock();
    Size size = app ? app->getWindow()->getContentRect().size : MakeSize(480, 768);

    this->paginatedView = std::make_shared<PaginatedCollectionView>(
        MakeRect(0, 0, size.width, size.height));
    this->paginatedView->setDataSource(this, this->weak_from_this());
    this->paginatedView->setDelegate(this, this->weak_from_this());
    this->paginatedView->setLayout(this->configuredLayout);
    this->paginatedView->setItemSize(this->configuredItemSize);
    this->paginatedView->setPaginationStyle(this->configuredPaginationStyle);

    this->view = this->paginatedView;
}

void CollectionViewController::viewDidLayoutSubviews() {
    ViewController::viewDidLayoutSubviews();

    // The container has finalized our root view's frame. Rebuild the internal
    // pagination layout with the actual dimensions and populate cells.
    if (this->paginatedView) {
        this->paginatedView->setPaginationStyle(this->configuredPaginationStyle);
        this->paginatedView->reloadData();
    }
}

void CollectionViewController::viewDidAppear() {
    ViewController::viewDidAppear();

    // Cells were created by reloadData() in viewDidLayoutSubviews(). For the
    // initial load, Window::addSubview's focus bootstrapping will have already
    // focused the first cell. For push transitions (View::addSubview, no
    // bootstrapping), ensure something is focused.
    if (this->paginatedView) {
        if (auto window = this->paginatedView->getWindow().lock()) {
            auto current = window->getFocusedView().lock();
            if (!current || current == window) {
                auto first = this->paginatedView->firstFocusableDescendant();
                if (first) first->becomeFocused();
            }
        }
    }
}

std::shared_ptr<PaginatedCollectionView> CollectionViewController::getPaginatedView() const {
    return this->paginatedView;
}
