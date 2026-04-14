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
 * @file CollectionViewDataSource.hpp
 * @brief Data source protocol for CollectionView.
 */

#pragma once

#include "Focus.hpp"
#include <cstddef>

class CollectionView;
class CollectionViewCell;

/**
 * @brief Abstract interface providing data to a CollectionView.
 *
 * Implement this interface to supply the number of items and the cell
 * for each item. The CollectionView calls these methods when loading
 * a page of items, passing itself as the first argument so that a single
 * data source can serve multiple collection views.
 * @ingroup views
 */
class CollectionViewDataSource {
public:
    /// @brief Return the total number of items in the data source.
    /// @param collectionView The collection view requesting this information.
    virtual size_t numberOfItems(CollectionView* collectionView) = 0;

    /**
     * @brief Create and return a cell for the item at the given index.
     * @param collectionView The collection view requesting this cell.
     * @param index The item index (0-based).
     * @param frame The frame rectangle to use for the created cell.
     * @return A shared_ptr to the cell representing this item.
     */
    virtual std::shared_ptr<CollectionViewCell> cellForItemAtIndex(CollectionView* collectionView, size_t index, Rect frame) = 0;

    /**
     * @brief Return the size for the item at the given index.
     * @param collectionView The collection view requesting this information.
     * @param index The item index (0-based).
     * @return The size for the item, or {0, 0} to use the collection view's
     *         itemSize (the default).
     *
     * Override this method to provide per-item sizing in VerticalList or
     * HorizontalList layouts. Only the dimension along the layout axis is
     * used: height for VerticalList, width for HorizontalList. The cross-axis
     * dimension is always filled from the collection view's frame (i.e. cells
     * span the full width of a vertical list, or the full height of a
     * horizontal list).
     *
     * This method is not called for Grid layouts, which always use itemSize.
     *
     * Item sizes must not change between calls to reloadData().
     */
    virtual Size sizeForItemAtIndex(CollectionView* collectionView, size_t index) {
        return {0, 0};
    }

    virtual ~CollectionViewDataSource() {}
};
