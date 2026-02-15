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

/**
 * @file CollectionViewController.hpp
 * @brief A convenience ViewController that manages a PaginatedCollectionView.
 *
 * Subclass CollectionViewController for the common case where a view controller's
 * entire content is a paginated collection. Override numberOfItems(),
 * cellForItemAtIndex(), and optionally didSelectItemAtIndex() — the simplified
 * API omits the CollectionView* parameter since there is only one collection.
 *
 * For view controllers that need multiple collection views, use the raw
 * CollectionViewDataSource and CollectionViewDelegate protocols directly.
 */

#pragma once

#include "ViewController.hpp"
#include "CollectionViewDataSource.hpp"
#include "CollectionViewDelegate.hpp"
#include "CollectionView.hpp"
#include "PaginatedCollectionView.hpp"

class CollectionViewCell;

/**
 * @brief A ViewController whose content is a single PaginatedCollectionView.
 *
 * The PaginatedCollectionView serves as the root view. Configuration (item size,
 * layout, pagination style) is stored and applied when the view is created.
 * Subclasses prepare their data in viewWillAppear(); the collection is reloaded
 * automatically in viewDidAppear() after the container has set the frame.
 */
class CollectionViewController : public ViewController,
                                  public CollectionViewDataSource,
                                  public CollectionViewDelegate {
public:
    CollectionViewController(std::shared_ptr<Application> application);

    /// @name Simplified Data Source API
    /// Subclasses override these. The CollectionView* parameter is omitted.
    /// @{
    virtual size_t numberOfItems() = 0;
    virtual std::shared_ptr<CollectionViewCell> cellForItemAtIndex(size_t index, Rect frame) = 0;
    /// @}

    /// @brief Called when the user taps an item. Override to handle selection.
    virtual void didSelectItemAtIndex(size_t index) {}

    /// @name Configuration
    /// Call these before the view appears (e.g. in your constructor).
    /// @{
    void setItemSize(Size size);
    void setLayout(CollectionViewLayout layout);
    void setPaginationStyle(PaginationStyle style);
    /// @}

    /// @brief Reload all items from the data source.
    void reloadData();

    void viewDidLayoutSubviews() override;
    void viewDidAppear() override;

protected:
    void createView() override;

    /// @brief Access the underlying PaginatedCollectionView.
    std::shared_ptr<PaginatedCollectionView> getPaginatedView() const;

private:
    // Bridge: forward protocol methods (with CollectionView*) to simplified API
    size_t numberOfItems(CollectionView*) final { return numberOfItems(); }
    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(
        CollectionView*, size_t index, Rect frame) final {
        return cellForItemAtIndex(index, frame);
    }
    void didSelectItemAtIndex(CollectionView*, size_t index) final {
        didSelectItemAtIndex(index);
    }

    std::shared_ptr<PaginatedCollectionView> paginatedView;
    Size configuredItemSize = {0, 0};
    CollectionViewLayout configuredLayout = CollectionViewLayout::VerticalList;
    PaginationStyle configuredPaginationStyle = PaginationStyle::Arrows;
};
