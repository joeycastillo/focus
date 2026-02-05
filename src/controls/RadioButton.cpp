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

#include "RadioButton.hpp"
#include "RadioGroup.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"

RadioButton::RadioButton(Rect rect, std::string text) : Control(rect) {
    this->text = text;
}

void RadioButton::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }

    // Resolve font
    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    GlyphProvider* providerPtr = nullptr;
    std::shared_ptr<GlyphProvider> glyphProvider;
    if (resolvedFont) {
        glyphProvider = resolvedFont->getSharedGlyphProvider();
        providerPtr = glyphProvider.get();
    }

    int lineHeight = 16;
    if (providerPtr) {
        lineHeight = providerPtr->getGlyphRowCount();
    }

    // Determine colors based on focus state
    int bgColor, fgColor;
    if (this->focused) {
        bgColor = this->foregroundColor;
        fgColor = this->backgroundColor;
    } else {
        bgColor = this->backgroundColor;
        fgColor = this->foregroundColor;
    }

    // Fill background
    this->canvas->clear(bgColor);

    // Draw radio indicator (circle)
    int indicatorSize = lineHeight;
    int radius = indicatorSize / 2 - 1;
    int centerX = 4 + indicatorSize / 2;
    int centerY = this->frame.size.height / 2;
    int gap = 8;

    // Draw outer circle
    this->canvas->drawCircle(centerX, centerY, radius, fgColor);

    // Fill inner circle if selected
    if (this->selected) {
        int innerRadius = radius - 3;
        if (innerRadius < 1) innerRadius = 1;
        this->canvas->fillCircle(centerX, centerY, innerRadius, fgColor);
    }

    // Draw label text to the right of indicator
    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }
    int textX = 4 + indicatorSize + gap;
    int textY = (this->frame.size.height - lineHeight) / 2;
    int textWidth = this->frame.size.width - textX;
    if (textWidth > 0) {
        Rect textRect = MakeRect(textX, textY, textWidth, lineHeight);
        this->canvas->drawText(textRect, fgColor, 1, this->text.c_str());
    }

    this->canvasValid = true;
}

void RadioButton::draw(int x, int y) {
    if (!this->canvasValid) this->renderCanvas();
    View::draw(x, y);
    if (this->canvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->canvas->getBufferData(), this->canvas->getRowBytes());
        }
    }
}

bool RadioButton::handleEvent(Event event) {
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        if (!this->selected) {
            this->selected = true;
            this->canvasValid = false;
            if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                this->setNeedsDisplayInRect(this->frame);
            }
            // Notify group to deselect others
            if (auto grp = this->group.lock()) {
                grp->_buttonSelected(this);
            }
            // Fire value changed action if registered
            auto it = this->actions.find(FOCUS_EVENT_VALUE_CHANGED);
            if (it != this->actions.end()) {
                Event valueEvent = {FOCUS_EVENT_VALUE_CHANGED, 1};
                it->second(valueEvent, this->weak_from_this());
            }
        }
        return true;
    }
    return View::handleEvent(event);
}

void RadioButton::didBecomeFocused() {
    Control::didBecomeFocused();
    this->canvasValid = false;
}

void RadioButton::didResignFocus() {
    Control::didResignFocus();
    this->canvasValid = false;
}

bool RadioButton::isSelected() const {
    return this->selected;
}

void RadioButton::setSelected(bool value) {
    if (this->selected != value) {
        this->selected = value;
        this->canvasValid = false;
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
        }
    }
}

void RadioButton::setText(std::string text) {
    this->text = text;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void RadioButton::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> RadioButton::getFont() const {
    return this->font;
}

void RadioButton::setGroup(std::shared_ptr<RadioGroup> group) {
    this->group = group;
}
