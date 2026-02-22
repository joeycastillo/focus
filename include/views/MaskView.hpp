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
 * @file MaskView.hpp
 * @brief View that renders a 1bpp mask, drawing foreground color where bits are set.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"

/**
 * @brief A view that renders foreground color through a 1bpp mask.
 *
 * Where mask bits are 1, the view's foreground color is drawn. Where mask
 * bits are 0, nothing is drawn (transparent). If the view is opaque, the
 * background color is drawn where mask bits are 0.
 *
 * The mask data is not owned by this view — it must remain valid for the
 * lifetime of the MaskView.
 */
class MaskView : public View {
public:
    /**
     * @brief Construct a mask view.
     * @param rect Frame rectangle (position and size).
     * @param mask Pointer to 1bpp MSB-first mask data. Must remain valid.
     */
    MaskView(Rect rect, const unsigned char *mask);
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
protected:
    const unsigned char *mask; ///< Pointer to the external mask data.
};
