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

#include "TabItem.hpp"
#include "CanvasView.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <algorithm>

TabItem::TabItem(Rect rect, std::string label) : Control(rect), label(label) {
}

void TabItem::setSelected(bool value) {
    if (this->selected != value) {
        bool wasHighlighted = this->selected || this->focused;
        Control::setSelected(value);
        bool isHighlighted = this->selected || this->focused;
        if (wasHighlighted != isHighlighted) {
            std::swap(this->backgroundColor, this->foregroundColor);
        }
    }
}

void TabItem::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
}

void TabItem::appearanceDidChange() {
    this->canvasValid = false;
}

void TabItem::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }

    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    GlyphProvider* providerPtr = nullptr;
    std::shared_ptr<GlyphProvider> glyphProvider;
    if (resolvedFont) {
        glyphProvider = resolvedFont->getSharedGlyphProvider();
        providerPtr = glyphProvider.get();
    }

    int lineHeight = providerPtr ? providerPtr->getGlyphRowCount() : 16;

    bool highlighted = this->selected || this->focused;

    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);

    // Unselected/unfocused tabs get a bottom border line
    if (!highlighted) {
        this->canvas->fillRect(0, this->frame.size.height - 1,
                               this->frame.size.width, 1, 1);
    }

    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }

    if (providerPtr && !this->label.empty()) {
        int textWidth = TextLayout::measureTextWidth(
            this->label.c_str(), 1, providerPtr);
        int textX = (this->frame.size.width - textWidth) / 2;
        if (textX < 0) textX = 0;
        int textY = (this->frame.size.height - lineHeight) / 2;
        Rect textRect = MakeRect(textX, textY, this->frame.size.width, lineHeight);
        this->canvas->drawText(textRect, 1, 1, this->label.c_str());
    }

    this->canvasValid = true;
}

void TabItem::drawContent(int x, int y, Rect clipRect) {
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

void TabItem::didBecomeFocused() {
    bool wasHighlighted = this->selected;
    Control::didBecomeFocused();
    if (!wasHighlighted) {
        std::swap(this->backgroundColor, this->foregroundColor);
    }
    this->canvasValid = false;
    if (this->onFocused) this->onFocused();
}

void TabItem::didResignFocus() {
    Control::didResignFocus();
    if (!this->selected) {
        std::swap(this->backgroundColor, this->foregroundColor);
    }
    this->canvasValid = false;
}
