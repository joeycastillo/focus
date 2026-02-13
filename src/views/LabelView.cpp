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

#include "LabelView.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"

LabelView::LabelView(Rect rect, std::string text) : View(rect) {
    this->text = text;
}

void LabelView::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }
    if (this->font) {
        this->canvas->setFont(this->font);
    }
    Rect layoutRect = MakeRect(0, 0, this->frame.size.width, this->frame.size.height);
    if (this->opaque) {
        // Opaque: render full background + text, blit everything
        this->canvas->clear(this->backgroundColor);
        this->canvas->drawText(layoutRect, this->foregroundColor,
                               this->textScale, this->text.c_str(), this->textAlignment);
    } else {
        // Non-opaque: render text as a mask (bits set where glyphs are)
        this->canvas->clear(0);
        this->canvas->drawText(layoutRect, 1,
                               this->textScale, this->text.c_str(), this->textAlignment);
    }
    this->canvasValid = true;
}

void LabelView::drawContent(int x, int y, Rect clipRect) {
    if (!this->canvasValid) this->renderCanvas();
    if (this->canvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            if (this->opaque) {
                display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                    this->frame.size.width, this->frame.size.height,
                                    this->canvas->getBufferData(), this->canvas->getRowBytes(), clipRect);
            } else {
                display->blitMasked(x + this->frame.origin.x, y + this->frame.origin.y,
                                    this->frame.size.width, this->frame.size.height,
                                    this->foregroundColor,
                                    this->canvas->getBufferData(), this->canvas->getRowBytes(), clipRect);
            }
        }
    }
}

void LabelView::setText(std::string text) {
    this->text = text;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void LabelView::setTextScale(uint8_t scale) {
    this->textScale = scale;
    this->canvasValid = false;
}

void LabelView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> LabelView::getFont() const {
    return this->font;
}

void LabelView::setTextAlignment(TextAlignment alignment) {
    this->textAlignment = alignment;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}