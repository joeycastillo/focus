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

#include "TabViewController.hpp"
#include "TabItem.hpp"
#include "StackView.hpp"
#include "Application.hpp"
#include "Window.hpp"
#include "Font.hpp"
#include "FocusMetrics.hpp"
#include <algorithm>

std::shared_ptr<TabViewController> TabViewController::create(
    std::shared_ptr<Application> application)
{
    auto tabVC = std::shared_ptr<TabViewController>(
        new TabViewController(application));
    return tabVC;
}

TabViewController::TabViewController(std::shared_ptr<Application> application)
    : ViewController(application)
{
}

void TabViewController::addTab(std::string label, std::shared_ptr<ViewController> viewController) {
    viewController->tabViewController = std::dynamic_pointer_cast<TabViewController>(
        this->shared_from_this());

    TabEntry entry;
    entry.label = label;
    entry.viewController = viewController;
    entry.tabItem = nullptr;
    this->tabs.push_back(entry);

    // If the view tree already exists, rebuild the tab bar to include the new tab
    if (this->view) {
        this->rebuildTabBar();
    }
}

int TabViewController::getTabBarHeight() const {
    // Per-instance font takes priority over global default
    if (this->font) {
        auto provider = this->font->getSharedGlyphProvider();
        if (provider) {
            int gh = provider->getGlyphRowCount();
            int padding = std::max(2, gh / 2);
            return gh + 2 * padding;
        }
    }
    return FocusMetrics::get().tabBarHeight;
}

void TabViewController::rebuildTabBar() {
    if (!this->tabBar) return;

    // Remove all children from the tab bar
    while (!this->tabBar->getSubviews().empty()) {
        this->tabBar->removeSubview(this->tabBar->getSubviews().back());
    }

    for (size_t i = 0; i < this->tabs.size(); i++) {
        auto item = std::make_shared<TabItem>(RectZero, this->tabs[i].label);
        if (this->font) item->setFont(this->font);
        item->setSelected(i == this->selectedIndex);

        size_t tabIndex = i;
        item->onFocused = [this, tabIndex]() {
            if (tabIndex != this->selectedIndex) {
                this->selectTab(tabIndex);
            }
        };
        item->setAction(
            [this, tabIndex](Event, std::weak_ptr<View>) {
                this->selectTab(tabIndex);
            },
            FOCUS_EVENT_TOUCH_UP_INSIDE);

        this->tabs[i].tabItem = item;
        this->tabBar->addSubview(item);
    }
}

void TabViewController::createView() {
    ViewController::createView();

    auto app = this->application.lock();
    if (!app) return;

    Size windowSize = app->getWindow()->getContentRect().size;

    this->view = std::make_shared<View>(MakeRect(0, 0, windowSize.width, windowSize.height));

    int barHeight = this->getTabBarHeight();

    // Tab bar — HStack of TabItems
    this->tabBar = std::make_shared<HStack>(
        MakeRect(0, 0, windowSize.width, barHeight));
    this->tabBar->setOpaque(false);
    this->view->addSubview(this->tabBar);

    // Content area below the tab bar
    this->contentArea = std::make_shared<View>(
        MakeRect(0, barHeight, windowSize.width, windowSize.height - barHeight));
    this->view->addSubview(this->contentArea);

    // Build tab items
    this->rebuildTabBar();
}

void TabViewController::viewWillAppear() {
    // Create our own container view
    ViewController::viewWillAppear();

    // Create the selected child's view and add it to the content area
    if (!this->tabs.empty() && this->contentArea) {
        if (this->selectedIndex >= this->tabs.size()) {
            this->selectedIndex = 0;
        }
        auto& selected = this->tabs[this->selectedIndex];
        selected.viewController->viewWillAppear();
        if (selected.viewController->view) {
            selected.viewController->view->setFrame(MakeRect(0, 0,
                this->contentArea->getFrame().size.width,
                this->contentArea->getFrame().size.height));
            selected.viewController->viewDidLayoutSubviews();
            this->contentArea->addSubview(selected.viewController->view);
        }
    }
}

void TabViewController::viewDidLayoutSubviews() {
    // Resize tab bar and content area to match our frame
    if (this->tabBar && this->contentArea) {
        int barHeight = this->getTabBarHeight();
        this->tabBar->setFrame(MakeRect(0, 0, this->view->getFrame().size.width, barHeight));
        this->contentArea->setFrame(MakeRect(0, barHeight,
            this->view->getFrame().size.width,
            this->view->getFrame().size.height - barHeight));
    }

    // Forward to selected child
    if (!this->tabs.empty() && this->selectedIndex < this->tabs.size()) {
        auto& selected = this->tabs[this->selectedIndex];
        if (selected.viewController->view) {
            selected.viewController->view->setFrame(MakeRect(0, 0,
                this->contentArea->getFrame().size.width,
                this->contentArea->getFrame().size.height));
            selected.viewController->viewDidLayoutSubviews();
        }
    }
}

void TabViewController::viewDidAppear() {
    if (!this->tabs.empty() && this->selectedIndex < this->tabs.size()) {
        this->tabs[this->selectedIndex].viewController->viewDidAppear();
    }
}

void TabViewController::viewWillDisappear() {
    if (!this->tabs.empty() && this->selectedIndex < this->tabs.size()) {
        this->tabs[this->selectedIndex].viewController->viewWillDisappear();
    }
}

void TabViewController::viewDidDisappear() {
    // Tear down the selected child's view
    if (!this->tabs.empty() && this->selectedIndex < this->tabs.size()) {
        auto& selected = this->tabs[this->selectedIndex];
        if (selected.viewController->view && this->contentArea) {
            this->contentArea->removeSubview(selected.viewController->view);
        }
        selected.viewController->viewDidDisappear();
    }

    this->tabBar.reset();
    this->contentArea.reset();
    ViewController::viewDidDisappear();
}

void TabViewController::selectTab(size_t index) {
    if (index >= this->tabs.size()) return;
    if (index == this->selectedIndex) return;
    if (!this->contentArea) return;

    this->transitionToTab(this->selectedIndex, index);
}

void TabViewController::transitionToTab(size_t oldIndex, size_t newIndex) {
    // Update tab item visuals
    if (oldIndex < this->tabs.size() && this->tabs[oldIndex].tabItem) {
        this->tabs[oldIndex].tabItem->setSelected(false);
    }
    if (newIndex < this->tabs.size() && this->tabs[newIndex].tabItem) {
        this->tabs[newIndex].tabItem->setSelected(true);
    }

    // Tear down the old child's view
    auto& oldTab = this->tabs[oldIndex];
    if (oldTab.viewController->view) {
        oldTab.viewController->viewWillDisappear();
        this->contentArea->removeSubview(oldTab.viewController->view);
        oldTab.viewController->viewDidDisappear();
    }

    this->selectedIndex = newIndex;

    // Set up the new child's view
    auto& newTab = this->tabs[newIndex];
    newTab.viewController->viewWillAppear();
    if (newTab.viewController->view) {
        newTab.viewController->view->setFrame(MakeRect(0, 0,
            this->contentArea->getFrame().size.width,
            this->contentArea->getFrame().size.height));
        newTab.viewController->viewDidLayoutSubviews();
        this->contentArea->addSubview(newTab.viewController->view);
    }
    newTab.viewController->viewDidAppear();

    if (std::shared_ptr<Window> window = this->view->getWindow().lock()) {
        this->view->setNeedsDisplayInRect(this->view->getFrame());
    }
}

size_t TabViewController::getSelectedTab() const {
    return this->selectedIndex;
}

size_t TabViewController::tabCount() const {
    return this->tabs.size();
}

void TabViewController::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    for (auto& tab : this->tabs) {
        if (tab.tabItem) tab.tabItem->setFont(font);
    }
}
