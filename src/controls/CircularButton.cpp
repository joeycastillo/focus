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

#include "CircularButton.hpp"
#include "CanvasView.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <algorithm>

CircularButton::CircularButton(Rect rect, std::string title) : Button(rect, std::move(title)) {
}

void CircularButton::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }

    this->canvas->clear(0);

    int cx = this->frame.size.width / 2;
    int cy = this->frame.size.height / 2;
    int radius = std::min(cx, cy) - 1;

    bool highlighted = this->selected || this->focused;

    if (highlighted) {
        this->canvas->fillCircle(cx, cy, radius, 1);
    } else {
        this->canvas->drawCircle(cx, cy, radius, 1);
    }

    // When highlighted, content is "cut out" from the filled circle (color=0).
    // When normal, content is drawn on transparent background (color=1).
    uint16_t contentColor = highlighted ? 0 : 1;

    const uint8_t* imgMask = this->getImage();
    Size imgSize = this->getImageSize();

    if (imgMask) {
        int imgX = (this->frame.size.width - imgSize.width) / 2;
        int imgY = (this->frame.size.height - imgSize.height) / 2;
        this->canvas->drawMask(imgX, imgY, imgSize.width, imgSize.height,
                               imgMask, (imgSize.width + 7) / 8, contentColor);
    } else {
        std::string title = this->getTitle();
        if (!title.empty()) {
            std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
            if (resolvedFont) {
                this->canvas->setFont(resolvedFont);
            }
            GlyphProvider* providerPtr = nullptr;
            std::shared_ptr<GlyphProvider> glyphProvider;
            if (resolvedFont) {
                glyphProvider = resolvedFont->getSharedGlyphProvider();
                providerPtr = glyphProvider.get();
            }
            int lineHeight = providerPtr ? providerPtr->getGlyphRowCount() : 16;
            int textY = (this->frame.size.height - lineHeight) / 2;
            Rect textRect = MakeRect(0, textY, this->frame.size.width, lineHeight);
            this->canvas->drawText(textRect, contentColor, 1, title.c_str(), TextAlignment::Center);
        }
    }

    this->canvasValid = true;
}
