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
 * @file ScrollView.hpp
 * @brief A viewport onto larger content, scrolled by setting a discrete offset.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"

namespace focus {

/**
 * @brief A viewport onto larger content, scrolled by setting a discrete offset.
 *
 * ScrollView clips its subtree to its frame and positions content by
 * offsetting its bounds origin. Add content with addSubview() as with any
 * view, set the scrollable extent with setContentSize(), then scroll with
 * setScrollOffset(). Offsets are clamped to the valid range on each axis,
 * and a call that does not change the offset does not redraw.
 *
 * ScrollView is not focusable and handles no events: the application maps
 * its input to setScrollOffset() calls. It draws no scroll indicators.
 * @ingroup views
 */
class ScrollView : public View {
public:
    /// @brief Construct a scroll view whose frame is the viewport.
    ScrollView(Rect rect);
    void setFrame(Rect rect) override;

    /// @brief Set the scrollable extent. The current offset is re-clamped against it.
    void setContentSize(Size size);
    /// @brief Get the scrollable extent.
    Size getContentSize() const;
    /**
     * @brief Scroll to the given offset.
     *
     * The offset is clamped to [0, contentSize - viewport] on each axis
     * (0 when content is smaller than the viewport). When the clamped
     * offset equals the current offset nothing is redrawn; otherwise the
     * viewport is invalidated and redraws at the new offset.
     * @param offset The desired offset in content coordinates.
     */
    void setScrollOffset(Point offset);
    /// @brief Get the current scroll offset.
    Point getScrollOffset() const;

private:
    Size contentSize = {};
    Point clampOffset(Point offset) const;
};

}  // namespace focus
