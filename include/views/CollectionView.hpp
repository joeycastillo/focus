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
 * @file CollectionView.hpp
 * @brief A paginated view that displays a list or grid of items from a data source.
 *
 * CollectionView requests views from a CollectionViewDataSource and arranges
 * them in a vertical list, horizontal list, or grid layout. Items are loaded
 * one page at a time, supporting pagination through large data sets.
 */

#pragma once

#include "View.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class CollectionViewDataSource;
class CollectionViewDelegate;

/// @brief Layout modes for CollectionView item arrangement.
enum class CollectionViewLayout {
    VerticalList,   ///< Items stacked vertically.
    HorizontalList, ///< Items arranged horizontally.
    Grid            ///< Items arranged in a grid.
};

/**
 * @brief A paginated collection of item views driven by a data source.
 *
 * Set a data source, item size, and layout, then call reloadData() to
 * populate the view. Use goToPage() to navigate between pages.
 */
class CollectionView : public View {
public:
    /// @brief Construct a collection view with the given frame.
    CollectionView(Rect rect);

    /**
     * @brief Set the data source that provides items.
     * @param dataSource Raw pointer to the data source (not retained).
     * @param owner Optional weak reference for lifetime tracking. When the
     *              owner expires, the data source is automatically cleared.
     */
    void setDataSource(CollectionViewDataSource* dataSource, std::weak_ptr<void> owner = {});

    /**
     * @brief Set the delegate for selection events.
     * @param delegate Raw pointer to the delegate (not retained).
     * @param owner Optional weak reference for lifetime tracking. When the
     *              owner expires, the delegate is automatically cleared.
     */
    void setDelegate(CollectionViewDelegate* delegate, std::weak_ptr<void> owner = {});

    /// @brief Set the layout mode (vertical list, horizontal list, or grid).
    void setLayout(CollectionViewLayout layout);
    /// @brief Set the size of each item cell.
    void setItemSize(Size size);
    /// @brief Set the spacing between items (applied between items, not at edges).
    void setItemSpacing(int spacing);

    /// @brief Reload all items from the data source, showing the first page.
    void reloadData();
    /// @brief Navigate to a specific page (0-based).
    void goToPage(size_t page);

    /// @brief Get the current page index (0-based).
    size_t getCurrentPage() const;
    /// @brief Get the total number of pages.
    size_t getPageCount() const;
    /// @brief Get how many items fit on one page.
    size_t getItemsPerPage() const;

    bool handleEvent(Event event) override;

    /// @brief Returns AccessibilityRole::List.
    AccessibilityRole accessibilityRole() const override;

private:
    CollectionViewDataSource* dataSource = nullptr;
    std::optional<std::weak_ptr<void>> dataSourceOwner;
    CollectionViewDelegate* delegate = nullptr;
    std::optional<std::weak_ptr<void>> delegateOwner;
    CollectionViewLayout layout = CollectionViewLayout::VerticalList;
    Size itemSize = {0, 0};
    int itemSpacing = 0;
    size_t currentPage = 0;

    bool variableItemSizes = false;
    std::vector<size_t> pageBoundaries;

    size_t calculateItemsPerPage() const;
    void computePageBoundaries();
    void loadPage(size_t page);
    void removeCurrentPageViews();
};
