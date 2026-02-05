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

/**
 * @brief Abstract interface providing data to a CollectionView.
 *
 * Implement this interface to supply the number of items and the view
 * for each item. The CollectionView calls these methods when loading
 * a page of items.
 */
class CollectionViewDataSource {
public:
    /// @brief Return the total number of items in the data source.
    virtual size_t numberOfItems() = 0;

    /**
     * @brief Create and return a view for the item at the given index.
     * @param index The item index (0-based).
     * @param frame The frame rectangle to use for the created view.
     * @return A shared_ptr to the view representing this item.
     */
    virtual std::shared_ptr<View> viewForItemAtIndex(size_t index, Rect frame) = 0;

    virtual ~CollectionViewDataSource() {}
};
