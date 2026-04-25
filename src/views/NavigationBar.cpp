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

#include "NavigationBar.hpp"
#include "Button.hpp"
#include "LabelView.hpp"
#include "StackView.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "Locale.hpp"
#include "FocusMetrics.hpp"

NavigationBar::NavigationBar(int width) : View(MakeRect(0, 0, width, getHeight())) {
    this->opaque = true;
}

std::shared_ptr<NavigationBar> NavigationBar::create(int width) {
    auto bar = std::shared_ptr<NavigationBar>(new NavigationBar(width));

    int height = getHeight();
    int padding = FocusMetrics::get().navBarPadding;
    int buttonWidth = FocusMetrics::get().navBarButtonWidth;

    // HStack handles horizontal layout and d-pad navigation
    auto layout = std::make_shared<HStack>(
        MakeRect(padding, padding, width - 2 * padding, height - 2 * padding));
    layout->setSpacing(padding);
    layout->setOpaque(false);

    // Back button, left-aligned, initially hidden
    bar->backButton = std::make_shared<Button>(
        MakeRect(0, 0, buttonWidth, 0), _LS("nav.back", "Back"));
    bar->backButton->setHidden(true);
    bar->backButton->setAction(
        [raw = bar.get()](Event, std::weak_ptr<View>) {
            if (raw->backAction) raw->backAction();
        },
        FOCUS_EVENT_TOUCH_UP_INSIDE);
    layout->addSubview(bar->backButton);

    // Title label, flexible width, centered
    bar->titleLabel = std::make_shared<LabelView>(RectZero, "");
    bar->titleLabel->setTextAlignment(TextAlignment::Center);
    bar->titleLabel->setOpaque(false);
    layout->addSubview(bar->titleLabel);

    // Right button, right-aligned, initially hidden
    bar->rightButton = std::make_shared<Button>(
        MakeRect(0, 0, buttonWidth, 0), "");
    bar->rightButton->setHidden(true);
    bar->rightButton->setAction(
        [raw = bar.get()](Event, std::weak_ptr<View>) {
            if (raw->rightAction) raw->rightAction();
        },
        FOCUS_EVENT_TOUCH_UP_INSIDE);
    layout->addSubview(bar->rightButton);

    bar->addSubview(layout);

    return bar;
}

int NavigationBar::getHeight() {
    return FocusMetrics::get().navBarHeight;
}

void NavigationBar::setTitle(const std::string& title) {
    this->titleLabel->setText(title);
}

void NavigationBar::setBackButtonVisible(bool visible) {
    this->backButton->setHidden(!visible);
}

void NavigationBar::setBackAction(std::function<void()> action) {
    this->backAction = action;
}

void NavigationBar::setRightButton(const std::string& title, std::function<void()> action) {
    this->rightAction = action;
    if (title.empty()) {
        this->rightButton->setHidden(true);
    } else {
        this->rightButton->setTitle(title);
        this->rightButton->setHidden(false);
    }
}

void NavigationBar::drawContent(int x, int y, Rect clipRect) {
    // Draw a 1px border along the bottom edge
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        int barBottom = y + this->frame.origin.y + this->frame.size.height - 1;
        display->fillRect(x + this->frame.origin.x, barBottom,
                          this->frame.size.width, 1,
                          this->foregroundColor, clipRect);
    }
}
