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

#include "TextField.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <algorithm>

namespace focus {

TextField::TextField(Rect rect) : Control(rect) {
}

std::string TextField::getText() const {
    return this->text;
}

void TextField::setText(std::string text) {
    this->text = text;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void TextField::setPlaceholder(std::string placeholder) {
    this->placeholder = placeholder;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void TextField::setMaxLength(size_t maxLength) {
    this->maxLength = maxLength;
}

void TextField::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> TextField::getFont() const {
    return this->font;
}

void TextField::setTextChangedCallback(std::function<void(std::string)> callback) {
    this->textChangedCallback = callback;
}

void TextField::insertText(const std::string& str) {
    if (this->maxLength > 0 && this->text.length() + str.length() > this->maxLength) {
        return;
    }
    this->text += str;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
    if (this->textChangedCallback) {
        this->textChangedCallback(this->text);
    }
}

void TextField::deleteBackward() {
    if (this->text.empty()) return;

    // Handle multi-byte UTF-8: find start of last character
    size_t pos = this->text.length() - 1;
    while (pos > 0 && (this->text[pos] & 0xC0) == 0x80) {
        pos--;
    }
    this->text.erase(pos);

    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
    if (this->textChangedCallback) {
        this->textChangedCallback(this->text);
    }
}

std::string TextField::getDisplayText() const {
    return this->text;
}

void TextField::renderCanvas() {
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

    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);

    // Draw border
    this->canvas->drawRect(0, 0, this->frame.size.width, this->frame.size.height, 1);

    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }

    // Draw text or placeholder
    int textPadding = 8;
    int textY = (this->frame.size.height - lineHeight) / 2;
    Rect textRect = MakeRect(textPadding, textY,
                             this->frame.size.width - 2 * textPadding, lineHeight);

    std::string displayText = this->getDisplayText();
    if (!displayText.empty()) {
        this->canvas->drawText(textRect, 1, 1, displayText.c_str());
    } else if (!this->placeholder.empty() && !this->focused) {
        this->canvas->drawText(textRect, 1, 1, this->placeholder.c_str());
    }

    // Draw cursor when focused
    if (this->focused && providerPtr) {
        int cursorX = textPadding;
        if (!displayText.empty()) {
            cursorX += TextLayout::measureTextWidth(displayText.c_str(), 1, providerPtr);
        }
        if (cursorX < this->frame.size.width - textPadding) {
            this->canvas->fillRect(cursorX, textY, 2, lineHeight, 1);
        }
    }

    this->canvasValid = true;
}

void TextField::drawContent(int x, int y, Rect clipRect) {
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

bool TextField::wantsKeyboardInput() const {
    return true;
}

void TextField::setKeyboardType(KeyboardType type) {
    this->_keyboardType = type;
}

KeyboardType TextField::keyboardType() const {
    return this->_keyboardType;
}

bool TextField::handleEvent(Event event) {
    if (!this->enabled) return false;
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        this->becomeFocused();
        this->fireAction(FOCUS_EVENT_TOUCH_DOWN, event);
        return true;
    }
    return View::handleEvent(event);
}

void TextField::didBecomeFocused() {
    Control::didBecomeFocused();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void TextField::didResignFocus() {
    Control::didResignFocus();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

AccessibilityRole TextField::accessibilityRole() const {
    return AccessibilityRole::TextField;
}

std::string TextField::accessibilityValue() const {
    return this->text;
}

}  // namespace focus
