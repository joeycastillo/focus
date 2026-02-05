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
 * @file TabbedView.hpp
 * @brief A view that organizes content into switchable tabs.
 *
 * TabbedView displays a row of tab labels at the top and shows the content
 * view associated with the selected tab. Tapping a tab label switches to
 * that tab's content. The tab bar is rendered to an internal CanvasView.
 */

#pragma once

#include "View.hpp"
#include <string>

class Font;
class CanvasView;

/**
 * @brief A tabbed container view with a touch-navigable tab bar.
 *
 * Add tabs with addTab(label, content). The first tab added is selected by
 * default. Only the selected tab's content view is visible and receives events.
 */
class TabbedView : public View {
public:
    /// @brief Construct a tabbed view with the given frame.
    TabbedView(Rect rect);

    /**
     * @brief Add a tab with a label and content view.
     * @param label Text displayed in the tab bar.
     * @param content The view to show when this tab is selected.
     */
    void addTab(std::string label, std::shared_ptr<View> content);

    /**
     * @brief Select a tab by index.
     * @param index Zero-based tab index.
     */
    void selectTab(size_t index);

    /// @brief Get the index of the currently selected tab.
    size_t getSelectedTab() const;

    /// @brief Set the font for tab labels. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);

    /// @brief Get the height of the tab bar area in pixels.
    int getTabBarHeight() const;

    void draw(int x, int y) override;
    /// @brief Handle touch events on the tab bar to switch tabs.
    bool handleEvent(Event event) override;

private:
    /// @brief Internal tab entry.
    struct Tab {
        std::string label;             ///< Tab label text.
        std::shared_ptr<View> content; ///< Tab content view.
    };
    std::vector<Tab> tabs;             ///< All tabs in order.
    size_t selectedIndex = 0;          ///< Currently selected tab index.
    std::shared_ptr<Font> font;        ///< Font for tab labels.
    std::shared_ptr<CanvasView> tabBarCanvas; ///< Canvas for rendering the tab bar.
    bool canvasValid = false;

    void renderTabBar();               ///< Render tab labels to the canvas.
};
