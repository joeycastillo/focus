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

#include "Checkbox.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include <algorithm>

Checkbox::Checkbox(Rect rect, std::string text) : Control(rect) {
    this->text = text;
}

void Checkbox::renderCanvas() {
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

    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);

    // Draw checkbox indicator
    int indicatorSize = lineHeight;
    int indicatorY = (this->frame.size.height - indicatorSize) / 2;
    int indicatorX = 4;
    int gap = 8;

    // Draw indicator border
    this->canvas->drawRect(indicatorX, indicatorY, indicatorSize, indicatorSize, 1);

    // Fill indicator if checked
    if (this->selected) {
        int inset = 3;
        this->canvas->fillRect(indicatorX + inset, indicatorY + inset,
                               indicatorSize - 2 * inset, indicatorSize - 2 * inset, 1);
    }

    // Draw label text to the right of indicator
    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }
    int textX = indicatorX + indicatorSize + gap;
    int textY = (this->frame.size.height - lineHeight) / 2;
    int textWidth = this->frame.size.width - textX;
    if (textWidth > 0) {
        Rect textRect = MakeRect(textX, textY, textWidth, lineHeight);
        this->canvas->drawText(textRect, 1, 1, this->text.c_str());
    }

    this->canvasValid = true;
}

void Checkbox::drawContent(int x, int y, Rect clipRect) {
    if (!this->canvasValid) this->renderCanvas();
    if (this->canvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            display->blitMasked(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->foregroundColor,
                                this->canvas->getBufferData(), this->canvas->getRowBytes(), clipRect);
        }
    }
}

bool Checkbox::handleEvent(Event event) {
    if (!this->enabled) return false;
    if (event.type == FOCUS_EVENT_TOUCH_DOWN || event.type == FOCUS_EVENT_SELECT) {
        this->setSelected(!this->selected);
        Event valueEvent = {FOCUS_EVENT_VALUE_CHANGED, this->selected ? 1 : 0, 0};
        this->fireAction(FOCUS_EVENT_VALUE_CHANGED, valueEvent);
        return true;
    }
    return View::handleEvent(event);
}

void Checkbox::didBecomeFocused() {
    Control::didBecomeFocused();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void Checkbox::didResignFocus() {
    Control::didResignFocus();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void Checkbox::appearanceDidChange() {
    this->canvasValid = false;
}

void Checkbox::setText(std::string text) {
    this->text = text;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void Checkbox::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> Checkbox::getFont() const {
    return this->font;
}

std::string Checkbox::accessibilityLabel() const {
    return this->text;
}

AccessibilityRole Checkbox::accessibilityRole() const {
    return AccessibilityRole::Checkbox;
}

std::string Checkbox::accessibilityValue() const {
    return this->selected ? "checked" : "unchecked";
}
