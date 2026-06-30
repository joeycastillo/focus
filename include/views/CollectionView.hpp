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
 * @brief Abstract base for views that display items from a CollectionViewDataSource.
 *
 * CollectionView is the type a CollectionViewDataSource and a CollectionViewDelegate
 * reason about (it is the sender passed to their methods), and the type apps hold
 * generically. It owns the data source/delegate wiring and the layout configuration
 * but defines no presentation strategy of its own — concrete subclasses decide how
 * items are realized. PagedCollectionView shows them one page at a time; a future
 * ScrollingCollectionView would window a continuous scroll.
 */

#pragma once

#include "View.hpp"
#include <cstddef>
#include <memory>
#include <optional>

namespace focus {

class CollectionViewDataSource;
class CollectionViewDelegate;

/// @brief Layout modes for CollectionView item arrangement.
enum class CollectionViewLayout {
    VerticalList,   ///< Items stacked vertically.
    HorizontalList, ///< Items arranged horizontally.
    Grid            ///< Items arranged in a grid.
};

/**
 * @brief Abstract base for a collection of item views driven by a data source.
 *
 * Set a data source, item size, and layout, then call reloadData() to populate the
 * view. CollectionView cannot be instantiated directly; construct a concrete
 * subclass such as PagedCollectionView.
 * @ingroup views
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

    /// @brief Get the layout mode.
    CollectionViewLayout getLayout() const { return this->layout; }
    /// @brief Get the item cell size.
    Size getItemSize() const { return this->itemSize; }
    /// @brief Get the spacing between items.
    int getItemSpacing() const { return this->itemSpacing; }

    /// @brief Reload all items from the data source. Subclasses define how items
    ///        are partitioned and realized.
    virtual void reloadData() = 0;

    /// @brief Returns AccessibilityRole::List.
    AccessibilityRole accessibilityRole() const override;

protected:
    CollectionViewDataSource* dataSource = nullptr;
    std::optional<std::weak_ptr<void>> dataSourceOwner;
    CollectionViewDelegate* delegate = nullptr;
    std::optional<std::weak_ptr<void>> delegateOwner;
    CollectionViewLayout layout = CollectionViewLayout::VerticalList;
    Size itemSize = {0, 0};
    int itemSpacing = 0;
};

}  // namespace focus
