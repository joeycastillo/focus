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
 * @file BorderedView.hpp
 * @brief View that draws a 1-pixel border around its frame.
 *
 * BorderedView renders a rectangular outline using a CanvasView for
 * the border pixels. Commonly used as a container for dialog boxes
 * and modal panels.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <memory>

class CanvasView;

/**
 * @brief A view that draws a 1-pixel rectangular border.
 *
 * The border is rendered to an internal CanvasView and cached. The
 * interior is filled with the view's background color.
 * @ingroup views
 */
class BorderedView : public View {
public:
    /**
     * @brief Construct a bordered view.
     * @param rect Frame rectangle defining the outer edge of the border.
     */
    BorderedView(Rect rect);
    void setFrame(Rect rect) override;
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
protected:
    std::shared_ptr<CanvasView> canvas; ///< Internal canvas for the border pixels.
    bool canvasValid = false;           ///< Whether the canvas needs re-rendering.

    /// @brief Render the border to the canvas. Override for custom border rendering.
    virtual void renderCanvas();
};
