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

#include "TabbedView.hpp"
#include "TabItem.hpp"
#include "Window.hpp"
#include "Font.hpp"

TabbedView::TabbedView(Rect rect) : View(rect) {
}

int TabbedView::getTabBarHeight() const {
    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    if (resolvedFont) {
        auto provider = resolvedFont->getSharedGlyphProvider();
        if (provider) {
            return provider->getGlyphRowCount() + 16; // line height + padding
        }
    }
    return 32; // fallback
}

void TabbedView::addTab(std::string label, std::shared_ptr<View> content) {
    Tab tab;
    tab.label = label;
    tab.content = content;
    tab.tabItem = nullptr; // created in rebuildTabBar
    this->tabs.push_back(tab);

    rebuildTabBar();
}

void TabbedView::rebuildTabBar() {
    // Remove all subviews (old tab bar + any content)
    while (!this->subviews.empty()) {
        this->removeSubview(this->subviews.back());
    }

    if (this->tabs.empty()) return;

    int barHeight = this->getTabBarHeight();

    // Create tab bar container with horizontal affinity for LEFT/RIGHT navigation
    this->tabBar = std::make_shared<View>(
        MakeRect(0, 0, this->frame.size.width, barHeight));
    this->tabBar->setDirectionalAffinity(DirectionalAffinityHorizontal);
    this->tabBar->setOpaque(false);

    int tabCount = (int)this->tabs.size();
    int tabWidth = this->frame.size.width / tabCount;

    for (int i = 0; i < tabCount; i++) {
        int tabX = i * tabWidth;
        int thisTabWidth = (i == tabCount - 1) ? (this->frame.size.width - tabX) : tabWidth;

        auto item = std::make_shared<TabItem>(
            MakeRect(tabX, 0, thisTabWidth, barHeight), this->tabs[i].label);
        if (this->font) item->setFont(this->font);
        item->setSelected((size_t)i == this->selectedIndex);

        // When a tab item receives focus via d-pad, switch to that tab immediately.
        size_t tabIndex = (size_t)i;
        item->onFocused = [this, tabIndex]() {
            if (tabIndex != this->selectedIndex) {
                this->selectTab(tabIndex);
            }
        };

        // Touch: tapping a tab item also switches to it.
        item->setAction(
            [this, tabIndex](Event, std::weak_ptr<View>) {
                this->selectTab(tabIndex);
            },
            FOCUS_EVENT_TOUCH_UP_INSIDE);

        this->tabs[i].tabItem = item;
        this->tabBar->addSubview(item);
    }

    // Add tab bar first (index 0), then content (index 1).
    // With vertical affinity (default), DOWN from tab bar enters content,
    // UP from content returns to tab bar.
    this->addSubview(this->tabBar);

    // Show the selected tab's content
    if (this->selectedIndex < this->tabs.size()) {
        auto& content = this->tabs[this->selectedIndex].content;
        content->setFrame(MakeRect(0, barHeight,
                                   this->frame.size.width,
                                   this->frame.size.height - barHeight));
        this->addSubview(content);
    }
}

void TabbedView::selectTab(size_t index) {
    if (index >= this->tabs.size()) return;
    if (index == this->selectedIndex) return;

    int barHeight = this->getTabBarHeight();

    // Deselect old tab item
    if (this->selectedIndex < this->tabs.size() && this->tabs[this->selectedIndex].tabItem) {
        this->tabs[this->selectedIndex].tabItem->setSelected(false);
    }

    // Remove old content
    if (this->selectedIndex < this->tabs.size()) {
        this->removeSubview(this->tabs[this->selectedIndex].content);
    }

    this->selectedIndex = index;

    // Select new tab item
    if (this->tabs[index].tabItem) {
        this->tabs[index].tabItem->setSelected(true);
    }

    // Add new content
    auto& newContent = this->tabs[index].content;
    newContent->setFrame(MakeRect(0, barHeight,
                                  this->frame.size.width,
                                  this->frame.size.height - barHeight));
    this->addSubview(newContent);

    if (this->onTabChanged) this->onTabChanged(index);

    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

size_t TabbedView::getSelectedTab() const {
    return this->selectedIndex;
}

void TabbedView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    for (auto& tab : this->tabs) {
        if (tab.tabItem) tab.tabItem->setFont(font);
    }
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}
