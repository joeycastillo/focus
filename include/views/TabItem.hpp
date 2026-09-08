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
 * @file TabItem.hpp
 * @brief A focusable tab label for use in TabViewController's tab bar.
 *
 * TabItem renders like a Button but has a `selected` state that keeps the
 * inverted appearance even when the item doesn't have focus. This allows
 * the active tab to remain visually distinct when focus moves into the
 * tab's content area below.
 */

#pragma once

#include "Control.hpp"
#include <functional>
#include <memory>
#include <string>

namespace focus {

class Font;
class CanvasView;

/**
 * @brief A tab label control that inverts when selected or focused.
 *
 * Used internally by TabViewController. When a TabItem receives focus via d-pad
 * navigation, TabViewController switches to that tab's content immediately.
 * @ingroup views
 */
class TabItem : public Control {
public:
    TabItem(Rect rect, std::string label);

    void setSelected(bool value) override;

    void setFont(std::shared_ptr<Font> font);

    /// @brief Called when this tab item receives focus. TabViewController uses
    /// this to switch tabs immediately on d-pad navigation (no SELECT needed).
    std::function<void()> onFocused;

    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
    void didBecomeFocused() override;
    void didResignFocus() override;
    void appearanceDidChange() override;

    /// @brief Returns the tab's label text.
    std::string accessibilityLabel() const override;
    /// @brief Returns AccessibilityRole::Tab.
    AccessibilityRole accessibilityRole() const override;
    /// @brief Returns "selected" for the active tab, otherwise empty.
    std::string accessibilityValue() const override;

protected:
    std::string label;                      ///< The tab label text.
    std::shared_ptr<Font> font;             ///< Custom font, or nullptr for system font.
    std::shared_ptr<CanvasView> canvas;     ///< Internal canvas for rendering.
    bool canvasValid = false;               ///< Whether the canvas needs re-rendering.
    /// @brief Render the tab label to the internal canvas.
    virtual void renderCanvas();
};

}  // namespace focus
