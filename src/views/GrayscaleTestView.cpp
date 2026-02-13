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

#include "GrayscaleTestView.hpp"

GrayscaleTestView::GrayscaleTestView(Rect rect) : View(rect) {
    canvas = std::make_shared<CanvasView>(Rect{Point{0, 0}, rect.size});
    canvas->setCanvasMode(DisplayMode::TwoBpp);
    canvas->setOpaque(false);
}

void GrayscaleTestView::renderContent() {
    int w = frame.size.width;
    int h = frame.size.height;
    int bandWidth = w / 4;

    uint16_t black = GrayscaleColor::Black();
    uint16_t darkGray = GrayscaleColor::DarkGray();
    uint16_t lightGray = GrayscaleColor::LightGray();
    uint16_t white = GrayscaleColor::White();

    // Draw four vertical bands
    canvas->fillRect(0, 0, bandWidth, h, black);
    canvas->fillRect(bandWidth, 0, bandWidth, h, darkGray);
    canvas->fillRect(bandWidth * 2, 0, bandWidth, h, lightGray);
    canvas->fillRect(bandWidth * 3, 0, w - bandWidth * 3, h, white);

    // Draw text labels in contrasting colors
    int textY = h / 2 - 8;
    int textH = 16;
    int margin = 4;

    canvas->drawText(Rect{Point{margin, textY}, Size{bandWidth - margin * 2, textH}},
                     white, 1, "Black", TextAlignmentCenter);
    canvas->drawText(Rect{Point{bandWidth + margin, textY}, Size{bandWidth - margin * 2, textH}},
                     white, 1, "Dark", TextAlignmentCenter);
    canvas->drawText(Rect{Point{bandWidth * 2 + margin, textY}, Size{bandWidth - margin * 2, textH}},
                     black, 1, "Light", TextAlignmentCenter);
    canvas->drawText(Rect{Point{bandWidth * 3 + margin, textY}, Size{bandWidth - margin * 2, textH}},
                     black, 1, "White", TextAlignmentCenter);

    needsRender = false;
}

void GrayscaleTestView::drawContent(int x, int y, Rect clipRect) {
    if (needsRender) {
        // Deferred from constructor: addSubview requires shared_from_this()
        if (subviews.empty()) {
            addSubview(canvas);
        }
        renderContent();
    }
}
