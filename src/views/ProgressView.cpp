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

#include "ProgressView.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"

void ProgressView::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }
    // Canvas is a shape mask: 0 = transparent, 1 = foreground
    this->canvas->clear(0);
    int filledWidth = (int)(this->frame.size.width * this->progress);
    if (filledWidth > 0) {
        this->canvas->fillRect(0, 0, filledWidth, this->frame.size.height, 1);
    }
    this->canvasValid = true;
}

void ProgressView::drawContent(int x, int y, Rect clipRect) {
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

void ProgressView::setProgress(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    this->progress = value;
    this->canvasValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

float ProgressView::getProgress() {
    return this->progress;
}

AccessibilityRole ProgressView::accessibilityRole() const {
    return AccessibilityRole::ProgressBar;
}

std::string ProgressView::accessibilityValue() const {
    int percent = (int)(this->progress * 100.0f + 0.5f);
    return std::to_string(percent) + "%";
}
