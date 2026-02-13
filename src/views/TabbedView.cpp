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
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"

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
    this->tabs.push_back(tab);
    this->canvasValid = false;

    // If this is the first tab, show it
    if (this->tabs.size() == 1) {
        int barHeight = this->getTabBarHeight();
        content->setFrame(MakeRect(0, barHeight,
                                   this->frame.size.width,
                                   this->frame.size.height - barHeight));
        this->addSubview(content);
    }
}

void TabbedView::selectTab(size_t index) {
    if (index >= this->tabs.size()) return;
    if (index == this->selectedIndex && !this->tabs.empty()) return;

    int barHeight = this->getTabBarHeight();

    // Remove current content
    if (this->selectedIndex < this->tabs.size()) {
        auto& currentContent = this->tabs[this->selectedIndex].content;
        this->removeSubview(currentContent);
    }

    this->selectedIndex = index;

    // Add new content
    auto& newContent = this->tabs[index].content;
    newContent->setFrame(MakeRect(0, barHeight,
                                  this->frame.size.width,
                                  this->frame.size.height - barHeight));
    this->addSubview(newContent);

    this->canvasValid = false;

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
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void TabbedView::renderTabBar() {
    int barHeight = this->getTabBarHeight();

    if (!this->tabBarCanvas) {
        this->tabBarCanvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, barHeight));
    }

    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    GlyphProvider* providerPtr = nullptr;
    std::shared_ptr<GlyphProvider> glyphProvider;
    if (resolvedFont) {
        glyphProvider = resolvedFont->getSharedGlyphProvider();
        providerPtr = glyphProvider.get();
    }

    int lineHeight = providerPtr ? providerPtr->getGlyphRowCount() : 16;

    // Clear tab bar background
    this->tabBarCanvas->clear(this->backgroundColor);

    if (resolvedFont) {
        this->tabBarCanvas->setFont(resolvedFont);
    }

    if (this->tabs.empty()) {
        this->canvasValid = true;
        return;
    }

    // Calculate tab widths — divide evenly
    int tabCount = (int)this->tabs.size();
    int tabWidth = this->frame.size.width / tabCount;

    for (int i = 0; i < tabCount; i++) {
        int tabX = i * tabWidth;
        int thisTabWidth = (i == tabCount - 1) ? (this->frame.size.width - tabX) : tabWidth;

        bool isSelected = ((size_t)i == this->selectedIndex);

        int bgColor, textColor;
        if (isSelected) {
            bgColor = this->foregroundColor;
            textColor = this->backgroundColor;
        } else {
            bgColor = this->backgroundColor;
            textColor = this->foregroundColor;
        }

        // Fill tab background
        int canvasBg = (bgColor == this->foregroundColor) ? 0 : 1;
        int canvasText = (textColor == this->foregroundColor) ? 0 : 1;
        this->tabBarCanvas->fillRect(tabX, 0, thisTabWidth, barHeight, canvasBg);

        // Draw tab border (bottom line for unselected, full border for selected)
        int borderColor = (this->foregroundColor == 0) ? 0 : 1;
        if (!isSelected) {
            // Bottom border line
            this->tabBarCanvas->fillRect(tabX, barHeight - 1, thisTabWidth, 1, borderColor);
        }
        // Vertical separator between tabs
        if (i > 0) {
            this->tabBarCanvas->fillRect(tabX, 0, 1, barHeight, borderColor);
        }

        // Draw tab label centered
        if (providerPtr) {
            int textWidth = TextLayout::measureTextWidth(
                this->tabs[i].label.c_str(), 1, providerPtr);
            int textX = tabX + (thisTabWidth - textWidth) / 2;
            if (textX < tabX) textX = tabX;
            int textY = (barHeight - lineHeight) / 2;
            Rect textRect = MakeRect(textX, textY, thisTabWidth, lineHeight);
            this->tabBarCanvas->drawText(textRect, canvasText, 1,
                                         this->tabs[i].label.c_str());
        }
    }

    // Draw outer border: top and sides of the tab bar, plus bottom line across full width
    int borderColor = (this->foregroundColor == 0) ? 0 : 1;
    this->tabBarCanvas->fillRect(0, 0, this->frame.size.width, 1, borderColor); // top
    this->tabBarCanvas->fillRect(0, 0, 1, barHeight, borderColor); // left
    this->tabBarCanvas->fillRect(this->frame.size.width - 1, 0, 1, barHeight, borderColor); // right

    this->canvasValid = true;
}

void TabbedView::drawContent(int x, int y, Rect clipRect) {
    if (!this->canvasValid) this->renderTabBar();

    if (this->tabBarCanvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->getTabBarHeight(),
                                this->tabBarCanvas->getBufferData(),
                                this->tabBarCanvas->getRowBytes(), clipRect);
        }
    }
}

bool TabbedView::handleEvent(Event event) {
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        // Convert window coordinates to local coordinates
        Point windowPoint = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
        Point localPoint = this->convertPointFromWindow(windowPoint);

        int barHeight = this->getTabBarHeight();
        if (localPoint.y >= 0 && localPoint.y < barHeight && !this->tabs.empty()) {
            // Touch is in the tab bar — determine which tab
            int tabCount = (int)this->tabs.size();
            int tabWidth = this->frame.size.width / tabCount;
            int tabIndex = localPoint.x / tabWidth;
            if (tabIndex >= tabCount) tabIndex = tabCount - 1;
            if (tabIndex < 0) tabIndex = 0;
            this->selectTab((size_t)tabIndex);
            return true;
        }
    }

    return View::handleEvent(event);
}
