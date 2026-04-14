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
 * @file CollectionViewCell.hpp
 * @brief A focusable container for use as an item in a CollectionView.
 *
 * CollectionViewCell is a Control (focusable, enabled/disabled) that starts
 * empty. Populate it with subviews — Labels, BitmapViews, or any other View —
 * to create rich collection view items. Register actions on the cell itself
 * to handle taps and other events.
 */

#pragma once

#include "Control.hpp"
#include <functional>

/**
 * @brief A focusable container view for CollectionView items.
 *
 * Unlike Button, which renders its own content to an internal canvas,
 * CollectionViewCell hosts arbitrary subviews. This makes it suitable
 * for composite layouts (e.g. an image alongside multiple labels).
 *
 * Focus appearance is not managed by the cell itself. Set a
 * CollectionViewDelegate on the owning CollectionView and implement
 * didFocusItemAtIndex / didUnfocusItemAtIndex to customize the visual
 * feedback for focused cells.
 * @ingroup controls
 */
class CollectionViewCell : public Control {
public:
    /// @brief Construct a cell with the given frame rectangle.
    CollectionViewCell(Rect rect);

    void didBecomeFocused() override;
    void didResignFocus() override;

    /// @brief Returns AccessibilityRole::ListItem.
    AccessibilityRole accessibilityRole() const override;

private:
    /// Internal callback set by CollectionView to route focus events to the delegate.
    std::function<void(CollectionViewCell&, bool)> onFocusChanged;
    friend class CollectionView;
};
