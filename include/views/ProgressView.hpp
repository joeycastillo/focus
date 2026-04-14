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
 * @file ProgressView.hpp
 * @brief View that displays a horizontal progress bar.
 *
 * ProgressView renders a rectangular track with a filled portion proportional
 * to its progress value (0.0 to 1.0). Used for displaying pagination progress,
 * loading status, etc.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <memory>

class CanvasView;

/**
 * @brief A horizontal progress bar view.
 *
 * Renders an outlined track with a filled portion. The filled width is
 * proportional to the progress value (0.0 = empty, 1.0 = full).
 * @ingroup views
 */
class ProgressView : public View {
public:
    /// @brief Construct a progress view with the given frame.
    ProgressView(Rect rect) : View(rect) {};
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;

    /**
     * @brief Set the progress value.
     * @param value Progress from 0.0 (empty) to 1.0 (full). Clamped to range;
     *              does not go to 11.
     */
    void setProgress(float value);
    void setFrame(Rect rect) override;

    /// @brief Get the current progress value.
    float getProgress();

    /// @brief Returns AccessibilityRole::ProgressBar.
    AccessibilityRole accessibilityRole() const override;
    /// @brief Returns the current progress as a percentage string.
    std::string accessibilityValue() const override;

protected:
    float progress = 0; ///< Current progress (0.0 to 1.0).
protected:
    std::shared_ptr<CanvasView> canvas; ///< Internal canvas for rendering.
    bool canvasValid = false;           ///< Whether the canvas needs re-rendering.

    /// @brief Render the progress bar to the canvas. Override for custom rendering.
    virtual void renderCanvas();
};
