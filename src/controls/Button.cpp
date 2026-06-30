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

namespace focus {

Button::Button(Rect rect, std::string title) : Control(rect) {
    this->titles[ControlState::Normal] = std::move(title);
}

void Button::renderCanvas() {
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

    int lineHeight = 16;
    if (providerPtr) {
        lineHeight = providerPtr->getGlyphRowCount();
    }

    this->canvas->clear(0);

    bool highlighted = this->selected || this->focused;
    if (!highlighted) {
        this->canvas->drawRect(0, 0, this->frame.size.width, this->frame.size.height, 1);
    }

    std::string title = this->getTitle();
    const uint8_t* imgMask = this->getImage();
    Size imgSize = this->getImageSize();

    if (imgMask && !title.empty()) {
        // Image + text: image on left, text to the right
        int gap = 8;
        int imgY = (this->frame.size.height - imgSize.height) / 2;
        int contentWidth = imgSize.width + gap;
        if (providerPtr) {
            contentWidth += TextLayout::measureTextWidth(title.c_str(), 1, providerPtr);
        }
        int startX = (this->frame.size.width - contentWidth) / 2;
        if (startX < 4) startX = 4;

        this->canvas->drawMask(startX, imgY, imgSize.width, imgSize.height,
                               imgMask, (imgSize.width + 7) / 8, 1);

        if (resolvedFont) {
            this->canvas->setFont(resolvedFont);
        }
        int textX = startX + imgSize.width + gap;
        int textY = (this->frame.size.height - lineHeight) / 2;
        int textWidth = this->frame.size.width - textX;
        if (textWidth > 0) {
            Rect textRect = MakeRect(textX, textY, textWidth, lineHeight);
            this->canvas->drawText(textRect, 1, 1, title.c_str());
        }
    } else if (imgMask) {
        // Image only: center it
        int imgX = (this->frame.size.width - imgSize.width) / 2;
        int imgY = (this->frame.size.height - imgSize.height) / 2;
        this->canvas->drawMask(imgX, imgY, imgSize.width, imgSize.height,
                               imgMask, (imgSize.width + 7) / 8, 1);
    } else if (!title.empty()) {
        // Text only: existing behavior
        if (resolvedFont) {
            this->canvas->setFont(resolvedFont);
        }
        int textWidth = 0;
        if (providerPtr) {
            textWidth = TextLayout::measureTextWidth(title.c_str(), 1, providerPtr);
        }

        int totalTextHeight = lineHeight;
        if (textWidth > this->frame.size.width) {
            int numLines = (textWidth + this->frame.size.width - 1) / this->frame.size.width;
            int lineSpacing = providerPtr ? TextLayout::calculateLineSpacing(providerPtr) : 2;
            totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
        } else if (title.find('\n') != std::string::npos) {
            int numLines = 1;
            for (char c : title) if (c == '\n') numLines++;
            int lineSpacing = providerPtr ? TextLayout::calculateLineSpacing(providerPtr) : 2;
            totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
        }

        int verticalOffset = (this->frame.size.height - totalTextHeight) / 2;
        Rect layoutRect = MakeRect(0, verticalOffset, this->frame.size.width, totalTextHeight);
        this->canvas->drawText(layoutRect, 1, 1, title.c_str(), TextAlignment::Center);
    }

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
    if (!this->selected) {
        std::swap(this->backgroundColor, this->foregroundColor);
    }
    this->canvasValid = false;
}

void Button::didResignFocus() {
    Control::didResignFocus();
    if (!this->selected) {
        std::swap(this->backgroundColor, this->foregroundColor);
    }
    this->canvasValid = false;
}

void Button::appearanceDidChange() {
    this->canvasValid = false;
}

void Button::setSelected(bool value) {
    if (this->selected != value) {
        bool wasHighlighted = this->selected || this->focused;
        Control::setSelected(value);
        bool isHighlighted = this->selected || this->focused;
        if (wasHighlighted != isHighlighted) {
            std::swap(this->backgroundColor, this->foregroundColor);
        }
    }
}

void Button::setTitle(const std::string& title, ControlState state) {
    this->titles[state] = title;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::string Button::getTitle() const {
    auto currentState = this->selected ? ControlState::Selected : ControlState::Normal;
    auto it = this->titles.find(currentState);
    if (it != this->titles.end()) return it->second;
    it = this->titles.find(ControlState::Normal);
    if (it != this->titles.end()) return it->second;
    return "";
}

void Button::setImage(const uint8_t* mask, Size size, ControlState state) {
    this->images[state] = {mask, size};
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

const uint8_t* Button::getImage() const {
    auto currentState = this->selected ? ControlState::Selected : ControlState::Normal;
    auto it = this->images.find(currentState);
    if (it != this->images.end()) return it->second.mask;
    it = this->images.find(ControlState::Normal);
    if (it != this->images.end()) return it->second.mask;
    return nullptr;
}

Size Button::getImageSize() const {
    auto currentState = this->selected ? ControlState::Selected : ControlState::Normal;
    auto it = this->images.find(currentState);
    if (it != this->images.end()) return it->second.size;
    it = this->images.find(ControlState::Normal);
    if (it != this->images.end()) return it->second.size;
    return {0, 0};
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
    return this->getTitle();
}

AccessibilityRole Button::accessibilityRole() const {
    return AccessibilityRole::Button;
}

std::string Button::accessibilityValue() const {
    if (this->selected) return "selected";
    return "";
}

}  // namespace focus
