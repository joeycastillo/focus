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
 * @file PagedCollectionView.hpp
 * @brief A CollectionView that displays its items one page at a time.
 *
 * PagedCollectionView is the paginated concrete CollectionView: it partitions the
 * data source's items into pages sized to its frame and builds one page of cells
 * at a time, supporting pagination through large data sets.
 */

#pragma once

#include "CollectionView.hpp"
#include <cstddef>
#include <vector>

namespace focus {

class CollectionViewCell;

/**
 * @brief A paginated collection of item views driven by a data source.
 *
 * Set a data source, item size, and layout, then call reloadData() to populate
 * the first page. Use goToPage() to navigate between pages. For pagination chrome
 * (arrows, footer, page indicator) wrap this in a PaginatedCollectionView.
 * @ingroup views
 */
class PagedCollectionView : public CollectionView {
public:
    /// @brief Construct a paged collection view with the given frame.
    PagedCollectionView(Rect rect);

    /// @brief Reload all items from the data source, showing the first page.
    void reloadData() override;
    /// @brief Navigate to a specific page (0-based).
    void goToPage(size_t page);

    /// @brief Get the current page index (0-based).
    size_t getCurrentPage() const;
    /// @brief Get the total number of pages.
    size_t getPageCount() const;
    /// @brief Get how many items fit on one page.
    size_t getItemsPerPage() const;

    void setFrame(Rect rect) override;
    bool handleEvent(Event event) override;

private:
    size_t currentPage = 0;
    bool dataLoaded = false;

    bool variableItemSizes = false;
    std::vector<size_t> pageBoundaries;

    size_t calculateItemsPerPage() const;
    void computePageBoundaries();
    void loadPage(size_t page);
    void removeCurrentPageViews();
    /// @brief Route a cell's select, long-press, and focus events to the delegate.
    void wireCell(std::shared_ptr<CollectionViewCell> cell, size_t index);
};

}  // namespace focus
