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
 * @file TabViewController.hpp
 * @brief A container view controller that manages switchable tabs of child view controllers.
 *
 * TabViewController displays a row of focusable tab items at the top and shows
 * the child view controller associated with the selected tab. Only the selected
 * tab's view exists at any given time — views are created lazily when a tab is
 * selected and destroyed when the user switches away.
 */

#pragma once

#include "ViewController.hpp"
#include <string>
#include <vector>
#include <memory>

namespace focus {

class Font;
class TabItem;
class HStack;

/**
 * @brief Container view controller with switchable tabs.
 *
 * Use the static create() factory method to construct. Add child view
 * controllers with addTab(). The first tab added is selected by default.
 *
 * Child view controllers can access their TabViewController via
 * ViewController::getTabViewController().
 *
 * Tab items carry index-stable accessibility identifiers, tab-item-0 through
 * tab-item-N-1 in addTab() order, and the tab bar itself carries tab-bar, so
 * scripted drivers and accessibility tools can address them without coordinates.
 * @ingroup viewcontrollers
 */
class TabViewController : public ViewController {
public:
    /**
     * @brief Create a tab view controller.
     * @param application The owning application.
     * @return A shared_ptr to the TabViewController.
     */
    static std::shared_ptr<TabViewController> create(
        std::shared_ptr<Application> application);

    /**
     * @brief Add a tab with a label and child view controller.
     * @param label Text displayed in the tab bar.
     * @param viewController The view controller to show when this tab is selected.
     */
    void addTab(std::string label, std::shared_ptr<ViewController> viewController);

    /**
     * @brief Select a tab by index.
     *
     * Runs the full lifecycle transition: tears down the old tab's view,
     * creates the new tab's view. While the tab controller is hidden, only
     * the selection changes, and the new tab appears with it. Does nothing
     * if index is already selected or out of bounds.
     *
     * @param index Zero-based tab index.
     */
    virtual void selectTab(size_t index);

    /// @brief Get the index of the currently selected tab.
    size_t getSelectedTab() const;

    /// @brief Get the number of tabs.
    size_t tabCount() const;

    /// @brief Set the font for tab labels. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);

    /// @brief Get the height of the tab bar area in pixels.
    int getTabBarHeight() const;

    // ViewController lifecycle overrides
    void viewWillAppear() override;
    void viewDidLayoutSubviews() override;
    void viewDidAppear() override;
    void viewWillDisappear() override;
    void viewDidDisappear() override;

protected:
    TabViewController(std::shared_ptr<Application> application);
    void createView() override;

    std::shared_ptr<HStack> tabBar;      ///< The tab bar container.
    std::shared_ptr<View> contentArea;   ///< The content area below the tab bar.

    /// @brief Transition from old selected tab to new selected tab.
    /// Override to customize how tab content is swapped in and out.
    virtual void transitionToTab(size_t oldIndex, size_t newIndex);

private:
    struct TabEntry {
        std::string label;
        std::shared_ptr<ViewController> viewController;
        std::shared_ptr<TabItem> tabItem;
    };

    std::vector<TabEntry> tabs;
    size_t selectedIndex = 0;
    std::shared_ptr<Font> font;

    /// @brief Rebuild the tab bar HStack from the current tabs list.
    void rebuildTabBar();
};

}  // namespace focus
