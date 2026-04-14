/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
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

#include "Button.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "TextLayout.hpp"
#include "Font.hpp"
#include <algorithm>
#include <vector>

Button::Button(Rect rect, std::string text) : Control(rect) {
    this->text = text;
}

void Button::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }

    // Resolve glyph provider from button's font or system font
    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    GlyphProvider* providerPtr = nullptr;
    std::shared_ptr<GlyphProvider> glyphProvider;
    if (resolvedFont) {
        glyphProvider = resolvedFont->getSharedGlyphProvider();
        providerPtr = glyphProvider.get();
    }

    int lineHeight = 16;
    int textWidth = 0;
    if (providerPtr) {
        lineHeight = providerPtr->getGlyphRowCount();
        textWidth = TextLayout::measureTextWidth(this->text.c_str(), 1, providerPtr);
    }

    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);

    // Draw border (only when not focused — focused buttons are filled solid)
    if (!this->focused) {
        this->canvas->drawRect(0, 0, this->frame.size.width, this->frame.size.height, 1);
    }

    // Set font on canvas for text rendering
    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }

    // Calculate total text height for vertical centering
    int totalTextHeight = lineHeight;
    if (textWidth > this->frame.size.width) {
        int numLines = (textWidth + this->frame.size.width - 1) / this->frame.size.width;
        int lineSpacing = providerPtr ? TextLayout::calculateLineSpacing(providerPtr) : 2;
        totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
    } else if (this->text.find('\n') != std::string::npos) {
        int numLines = 1;
        for (char c : this->text) if (c == '\n') numLines++;
        int lineSpacing = providerPtr ? TextLayout::calculateLineSpacing(providerPtr) : 2;
        totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
    }

    int verticalOffset = (this->frame.size.height - totalTextHeight) / 2;
    Rect layoutRect = MakeRect(0, verticalOffset, this->frame.size.width, totalTextHeight);
    this->canvas->drawText(layoutRect, 1, 1, this->text.c_str(), TextAlignment::Center);

    this->canvasValid = true;
}

void Button::drawContent(int x, int y, Rect clipRect) {
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

void Button::didBecomeFocused() {
    Control::didBecomeFocused();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void Button::didResignFocus() {
    Control::didResignFocus();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void Button::appearanceDidChange() {
    this->canvasValid = false;
}

void Button::setText(const std::string& text) {
    this->text = text;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void Button::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> Button::getFont() const {
    return this->font;
}

std::string Button::accessibilityLabel() const {
    return this->text;
}

AccessibilityRole Button::accessibilityRole() const {
    return AccessibilityRole::Button;
}
