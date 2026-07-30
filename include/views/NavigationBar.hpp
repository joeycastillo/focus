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
 * @file NavigationBar.hpp
 * @brief A bar view with a back button and a title label.
 *
 * NavigationBar is used by NavigationViewController to display the current
 * view controller's title and provide a back button for popping the stack.
 */

#pragma once

#include "View.hpp"
#include "LabelView.hpp"
#include <string>
#include <functional>
#include <memory>

namespace focus {

class Button;
class HStack;

/**
 * @brief A navigation bar with a back button and centered title.
 *
 * The back button is hidden by default and shown when the navigation stack
 * has more than one view controller. A 1px border is drawn at the bottom
 * of the bar.
 * @ingroup views
 */
class NavigationBar : public View {
public:
    /// @brief Create a navigation bar spanning the given width.
    /// @param width The width of the bar (typically the full screen width).
    static std::shared_ptr<NavigationBar> create(int width);

    /// @brief Set the title text displayed in the bar.
    void setTitle(const std::string& title);
    /// @brief Get the title text displayed in the bar.
    std::string getTitle() const { return this->titleLabel ? this->titleLabel->getText() : ""; }

    /// @brief Show or hide the back button.
    void setBackButtonVisible(bool visible);

    /// @brief Register a callback invoked when the back button is tapped.
    void setBackAction(std::function<void()> action);

    /// @brief Set the right button's title and action. The button is hidden when title is empty.
    void setRightButton(const std::string& title, std::function<void()> action);

    /// @brief Get the height of the navigation bar in pixels.
    static int getHeight();

    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;

private:
    NavigationBar(int width);

    /// Rebuild the layout stack so it contains exactly the present slots.
    void setSlotPresence(bool back, bool right);

    std::shared_ptr<HStack> layout;
    std::shared_ptr<Button> backButton;
    std::shared_ptr<LabelView> titleLabel;
    std::shared_ptr<Button> rightButton;
    std::function<void()> backAction;
    std::function<void()> rightAction;
    bool backPresent = false;
    bool rightPresent = false;
};

}  // namespace focus
