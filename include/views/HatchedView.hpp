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

/**
 * @file HatchedView.hpp
 * @brief View that draws a semi-transparent hatched overlay.
 *
 * HatchedView renders a dense mask pattern over its frame, producing
 * a visual dimming effect. Used by Application::presentViewController() to
 * dim the content behind modal view controllers.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <vector>

/**
 * @brief A view that draws a dither pattern for visual dimming.
 *
 * Diagonal lines are drawn in the specified color, creating a ~50% density
 * hatched overlay. The mask is pre-computed on construction.
 * @ingroup views
 */
class HatchedView : public View {
public:
    /**
     * @brief Construct a hatched overlay view.
     * @param rect Frame rectangle to cover.
     * @param color The color to draw the hatched pixels in.
     */
    HatchedView(Rect rect, uint16_t color);
    void setFrame(Rect rect) override;
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
private:
    int maskRowBytes;             ///< Bytes per row in the mask buffer.
    std::vector<uint8_t> mask;    ///< Pre-computed checkerboard mask bitmap.
};
