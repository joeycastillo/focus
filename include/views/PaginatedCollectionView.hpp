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
 * @file PaginatedCollectionView.hpp
 * @brief A CollectionView wrapper with built-in pagination indicators.
 *
 * PaginatedCollectionView composes a CollectionView with automatic pagination
 * controls. Choose from lightweight arrow indicators at the edges (Arrows style)
 * or a traditional button strip with page numbers (Footer style).
 */

#pragma once

#include "View.hpp"
#include "CollectionView.hpp"
#include <memory>

class CollectionViewDataSource;
class CollectionViewDelegate;
class CanvasView;
class Button;
class LabelView;

/// @brief Visual style for pagination controls.
enum class PaginationStyle {
    None,    ///< No visible indicators (programmatic navigation only).
    Arrows,  ///< Small arrow indicators at the edges of the collection.
    Footer,  ///< "< Prev" / "Page X of Y" / "Next >" button strip at the bottom.
};

/**
 * @brief A paginated collection view with built-in navigation controls.
 *
 * Wraps a CollectionView and adds pagination indicators that automatically
 * update as pages change. For vertical layouts with Arrows style, up/down
 * arrows appear at the top and bottom edges. For horizontal layouts, left/right
 * arrows appear at the left and right edges. The Footer style always places a
 * horizontal button strip at the bottom regardless of layout direction.
 */
class PaginatedCollectionView : public View {
public:
    PaginatedCollectionView(Rect rect);

    /// @brief Set the data source that provides items. Not retained (raw pointer).
    void setDataSource(CollectionViewDataSource *dataSource, std::weak_ptr<void> owner = {});
    /// @brief Set the delegate for selection events.
    void setDelegate(CollectionViewDelegate *delegate, std::weak_ptr<void> owner = {});
    /// @brief Set the layout mode (vertical list, horizontal list, or grid).
    void setLayout(CollectionViewLayout layout);
    /// @brief Set the size of each item cell.
    void setItemSize(Size size);

    /// @brief Set the visual style for pagination controls.
    void setPaginationStyle(PaginationStyle style);

    /// @brief Reload all items from the data source, showing the first page.
    void reloadData();
    /// @brief Navigate to the next page (no-op if already on the last page).
    void goToNextPage();
    /// @brief Navigate to the previous page (no-op if already on the first page).
    void goToPreviousPage();
    /// @brief Navigate to a specific page (clamped to valid range).
    void goToPage(size_t page);

    /// @brief Get the current page index (0-based).
    size_t getCurrentPage() const;
    /// @brief Get the total number of pages.
    size_t getPageCount() const;
    /// @brief Get how many items fit on one page.
    size_t getItemsPerPage() const;

    bool handleEvent(Event event) override;

private:
    std::shared_ptr<CollectionView> collectionView;

    // Arrows style
    std::shared_ptr<CanvasView> beforeIndicator;
    std::shared_ptr<CanvasView> afterIndicator;

    // Footer style
    std::shared_ptr<View> footerContainer;
    std::shared_ptr<Button> prevButton;
    std::shared_ptr<LabelView> pageLabel;
    std::shared_ptr<Button> nextButton;

    PaginationStyle paginationStyle = PaginationStyle::None;
    CollectionViewLayout currentLayout = CollectionViewLayout::VerticalList;

    static constexpr int kArrowThickness = 24;
    static constexpr int kFooterThickness = 48;
    static constexpr int kFooterGap = 8;

    void rebuildLayout();
    void updateIndicators();
    void drawArrow(std::shared_ptr<CanvasView> canvas, bool forward);
    void updateFooterLabel();
};
