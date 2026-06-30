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

#include "Slider.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace focus {

Slider::Slider(Rect rect, std::string label) : Control(rect), label(label) {
}

int Slider::getTrackX() const {
    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    int labelWidth = 0;
    if (resolvedFont) {
        auto provider = resolvedFont->getSharedGlyphProvider();
        if (provider) {
            labelWidth = TextLayout::measureTextWidth(
                this->label.c_str(), 1, provider.get());
        }
    }
    return 4 + labelWidth + 8;
}

int Slider::getTrackWidth() const {
    return this->frame.size.width - getTrackX() - 4;
}

void Slider::renderCanvas() {
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

    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);

    // Draw label text on the left
    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }
    int textX = 4;
    int textY = (this->frame.size.height - lineHeight) / 2;
    if (!this->label.empty() && providerPtr) {
        int labelWidth = TextLayout::measureTextWidth(
            this->label.c_str(), 1, providerPtr);
        Rect textRect = MakeRect(textX, textY, labelWidth, lineHeight);
        this->canvas->drawText(textRect, 1, 1, this->label.c_str());
    }

    // Draw track to the right of the label
    int trackX = getTrackX();
    int trackWidth = this->frame.size.width - trackX - 4;
    int trackHeight = lineHeight;
    int trackY = (this->frame.size.height - trackHeight) / 2;

    if (trackWidth > 0) {
        // Track outline
        this->canvas->drawRect(trackX, trackY, trackWidth, trackHeight, 1);

        // Filled portion (quantize to step grid if step is set)
        float displayValue = this->value;
        if (this->step > 0.0f) {
            displayValue = roundf(displayValue / this->step) * this->step;
            if (displayValue > 1.0f) displayValue = 1.0f;
        }
        int filledWidth = (int)(trackWidth * displayValue);
        if (filledWidth > 0) {
            this->canvas->fillRect(trackX, trackY, filledWidth, trackHeight, 1);
        }
    }

    this->canvasValid = true;
}

void Slider::drawContent(int x, int y, Rect clipRect) {
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

bool Slider::handleEvent(Event event) {
    if (!this->enabled) return false;
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        Point windowPoint = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
        Point localPoint = this->convertPointFromWindow(windowPoint);

        int trackX = getTrackX();
        int trackWidth = getTrackWidth();

        if (trackWidth > 0) {
            float newValue = (float)(localPoint.x - trackX) / (float)trackWidth;
            if (newValue < 0.0f) newValue = 0.0f;
            if (newValue > 1.0f) newValue = 1.0f;
            this->value = newValue;
            this->canvasValid = false;
            if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                this->setNeedsDisplayInRect(this->frame);
            }
            // Fire value changed action
            auto it = this->actions.find(FOCUS_EVENT_VALUE_CHANGED);
            if (it != this->actions.end()) {
                int32_t valueBits; memcpy(&valueBits, &this->value, sizeof(valueBits));
                Event valueEvent = {FOCUS_EVENT_VALUE_CHANGED, valueBits, 0};
                it->second.callback(valueEvent, this->weak_from_this());
            }
        }
        return true;
    }
    // LEFT/RIGHT adjust value when focused; UP/DOWN pass through for navigation.
    if (event.type == FOCUS_EVENT_DIRECTION_LEFT || event.type == FOCUS_EVENT_DIRECTION_RIGHT) {
        float delta = (event.type == FOCUS_EVENT_DIRECTION_RIGHT) ? this->step : -this->step;
        float newValue = this->value + delta;
        if (newValue < 0.0f) newValue = 0.0f;
        if (newValue > 1.0f) newValue = 1.0f;
        if (newValue != this->value) {
            this->value = newValue;
            this->canvasValid = false;
            if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                this->setNeedsDisplayInRect(this->frame);
            }
            auto it = this->actions.find(FOCUS_EVENT_VALUE_CHANGED);
            if (it != this->actions.end()) {
                int32_t valueBits; memcpy(&valueBits, &this->value, sizeof(valueBits));
                Event valueEvent = {FOCUS_EVENT_VALUE_CHANGED, valueBits, 0};
                it->second.callback(valueEvent, this->weak_from_this());
            }
        }
        return true;
    }
    return View::handleEvent(event);
}

void Slider::didBecomeFocused() {
    Control::didBecomeFocused();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

void Slider::didResignFocus() {
    Control::didResignFocus();
    std::swap(this->backgroundColor, this->foregroundColor);
    this->canvasValid = false;
}

float Slider::getValue() const {
    return this->value;
}

void Slider::setValue(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    if (this->value != value) {
        this->value = value;
        this->canvasValid = false;
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
        }
    }
}

void Slider::setLabel(std::string label) {
    this->label = label;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void Slider::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> Slider::getFont() const {
    return this->font;
}

void Slider::setStep(float step) {
    if (step < 0.001f) step = 0.001f;
    this->step = step;
}

AccessibilityRole Slider::accessibilityRole() const {
    return AccessibilityRole::Slider;
}

std::string Slider::accessibilityValue() const {
    int percent = (int)(this->value * 100.0f + 0.5f);
    return std::to_string(percent) + "%";
}

}  // namespace focus
