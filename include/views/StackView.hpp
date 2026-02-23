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
 * @file StackView.hpp
 * @brief VStack and HStack layout views for automatic child positioning.
 *
 * StackView arranges its subviews sequentially along a single axis:
 * - **VStack**: top-to-bottom (vertical axis)
 * - **HStack**: left-to-right (horizontal axis)
 *
 * Each child's size along the layout axis is determined by its frame:
 * - **Fixed**: frame height > 0 (VStack) or frame width > 0 (HStack).
 *   The child gets exactly that many pixels.
 * - **Flexible**: frame height == 0 (VStack) or frame width == 0 (HStack).
 *   The child receives an equal share of the remaining space after fixed
 *   children and spacing are subtracted.
 *
 * The cross-axis dimension always fills the stack's width (VStack) or
 * height (HStack). Children handle their own internal alignment.
 *
 * Spacing is applied between items only, not at edges. Optional margins
 * inset children from the stack's edges on all four sides.
 *
 * Hidden children reserve their space in the layout but are not drawn.
 * To collapse a child's space, remove it with removeSubview() — layout
 * updates automatically.
 *
 * Focus navigation affinity is set automatically: DirectionalAffinityVertical
 * for VStack, DirectionalAffinityHorizontal for HStack.
 */

#pragma once

#include "View.hpp"

/**
 * @brief Base class for stack layout views.
 *
 * Use VStack or HStack instead of instantiating StackView directly.
 * These classes are not designed for subclassing — they provide a fixed
 * layout algorithm (equal-share flexible + fixed-size children along one
 * axis). For more complex layouts, nest VStack and HStack within each
 * other, or subclass View directly and position children manually in a
 * setFrame() override.
 */
class StackView : public View {
public:
    /// @brief The axis along which children are arranged.
    enum class Axis { Vertical, Horizontal };

    /// @brief Construct a stack view with the given frame and axis.
    StackView(Rect rect, Axis axis);

    /// @brief Set the spacing between children (applied between items, not at edges).
    void setSpacing(int spacing);

    /// @brief Get the current spacing between children.
    int getSpacing() const;

    /// @brief Set margins on all four sides (top, right, bottom, left).
    void setMargins(int top, int right, int bottom, int left);

    /// @brief Set uniform margins on all four sides.
    void setMargins(int uniform);

    /// @brief Recalculate and apply frames for all children.
    void layoutSubviews();

    void addSubview(std::shared_ptr<View> view) override;
    void removeSubview(std::shared_ptr<View> view) override;
    void setFrame(Rect rect) override;

private:
    Axis axis;
    int spacing = 0;
    int marginTop = 0;
    int marginRight = 0;
    int marginBottom = 0;
    int marginLeft = 0;
    std::vector<int> preferredSizes; ///< Axis-size recorded at addSubview time (0 = flexible).
};

/**
 * @brief A vertical stack that arranges children top-to-bottom.
 *
 * Children with frame height > 0 get fixed height. Children with frame
 * height == 0 share the remaining space equally.
 */
class VStack : public StackView {
public:
    VStack(Rect rect) : StackView(rect, Axis::Vertical) {}
};

/**
 * @brief A horizontal stack that arranges children left-to-right.
 *
 * Children with frame width > 0 get fixed width. Children with frame
 * width == 0 share the remaining space equally.
 */
class HStack : public StackView {
public:
    HStack(Rect rect) : StackView(rect, Axis::Horizontal) {}
};
