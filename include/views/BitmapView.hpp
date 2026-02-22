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
 * @file BitmapView.hpp
 * @brief View that displays a 1bpp bitmap image opaquely.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"

/**
 * @brief A view that renders a 1bpp bitmap opaquely to the display.
 *
 * Every pixel in the bitmap is drawn: set bits (1) produce white, clear
 * bits (0) produce black. This matches the display's native blitOpaque
 * convention.
 *
 * The bitmap data is not owned by this view — it must remain valid for
 * the lifetime of the BitmapView.
 */
class BitmapView : public View {
public:
    /**
     * @brief Construct a bitmap view.
     * @param rect Frame rectangle (position and size).
     * @param bitmap Pointer to 1bpp MSB-first bitmap data. Must remain valid.
     */
    BitmapView(Rect rect, const unsigned char *bitmap);
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
protected:
    const unsigned char *bitmap; ///< Pointer to the external bitmap data.
};
