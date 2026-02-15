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
#include "Display.hpp"
#include "Font.hpp"

NavigationBar::NavigationBar(int width) : View(MakeRect(0, 0, width, getHeight())) {
    this->opaque = true;

    int height = getHeight();
    int padding = 8;
    int backButtonWidth = 80;
    int backButtonHeight = height - 2 * padding;

    // Back button, left-aligned, initially hidden
    this->backButton = std::make_shared<Button>(
        MakeRect(padding, padding, backButtonWidth, backButtonHeight), "Back");
    this->backButton->setHidden(true);
    this->backButton->setAction(
        [this](Event, std::weak_ptr<View>) {
            if (this->backAction) this->backAction();
        },
        FOCUS_EVENT_TOUCH_UP_INSIDE);
    this->addSubview(this->backButton);

    // Title label, centered in the space between the back button area and right edge
    int titleX = padding + backButtonWidth + padding;
    int titleWidth = width - 2 * titleX; // symmetrical margin
    if (titleWidth < 0) titleWidth = 0;
    this->titleLabel = std::make_shared<LabelView>(
        MakeRect(titleX, padding, titleWidth, backButtonHeight), "");
    this->titleLabel->setTextAlignment(TextAlignmentCenter);
    this->titleLabel->setOpaque(false);
    this->addSubview(this->titleLabel);
}

int NavigationBar::getHeight() {
    auto font = Font::systemFont();
    if (font) {
        return font->getGlyphRowCount() + 16;
    }
    return 48;
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

void NavigationBar::drawContent(int x, int y, Rect clipRect) {
    // Draw a 1px border along the bottom edge
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        int barBottom = y + this->frame.origin.y + this->frame.size.height - 1;
        display->fillRect(x + this->frame.origin.x, barBottom,
                          this->frame.size.width, 1,
                          this->foregroundColor, clipRect);
    }
}
