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

    // Determine colors based on focus state
    int bgColor, textColor, borderColor;
    if (this->focused) {
        bgColor = this->foregroundColor;
        textColor = this->backgroundColor;
        borderColor = this->foregroundColor;
    } else {
        bgColor = this->backgroundColor;
        textColor = this->foregroundColor;
        borderColor = this->foregroundColor;
    }

    int canvasBg = (bgColor == this->foregroundColor) ? 0 : 1;
    int canvasText = (textColor == this->foregroundColor) ? 0 : 1;
    int canvasBorder = (borderColor == this->foregroundColor) ? 0 : 1;

    // Fill background
    this->canvas->clear(canvasBg);

    // Draw border
    this->canvas->drawRect(0, 0, this->frame.size.width, this->frame.size.height, canvasBorder);

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
        this->canvas->drawText(textRect, canvasText, 1, displayText.c_str());
    } else if (!this->placeholder.empty() && !this->focused) {
        // Draw placeholder in a lighter style (we only have 1bpp so just use the text color)
        this->canvas->drawText(textRect, canvasText, 1, this->placeholder.c_str());
    }

    // Draw cursor when focused
    if (this->focused && providerPtr) {
        int cursorX = textPadding;
        if (!displayText.empty()) {
            cursorX += TextLayout::measureTextWidth(displayText.c_str(), 1, providerPtr);
        }
        if (cursorX < this->frame.size.width - textPadding) {
            this->canvas->fillRect(cursorX, textY, 2, lineHeight, canvasText);
        }
    }

    this->canvasValid = true;
}

void TextField::draw(int x, int y) {
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

bool TextField::wantsKeyboardInput() {
    return true;
}

void TextField::setKeyboardType(KeyboardType type) {
    this->_keyboardType = type;
}

KeyboardType TextField::keyboardType() {
    return this->_keyboardType;
}

bool TextField::handleEvent(Event event) {
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        this->becomeFocused();
        if (this->actions.count(FOCUS_EVENT_TOUCH_DOWN)) {
            this->actions[FOCUS_EVENT_TOUCH_DOWN].callback(event, this->shared_from_this());
        }
        return true;
    }
    return View::handleEvent(event);
}

void TextField::didBecomeFocused() {
    Control::didBecomeFocused();
    this->canvasValid = false;
}

void TextField::didResignFocus() {
    Control::didResignFocus();
    this->canvasValid = false;
}
