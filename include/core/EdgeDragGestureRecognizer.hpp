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

/**
 * @file EdgeDragGestureRecognizer.hpp
 * @brief Recognizes drag gestures starting from a specific screen region.
 *
 * EdgeDragGestureRecognizer activates when a touch begins inside its
 * activation region and moves beyond a threshold distance. Typical use:
 * attach to a window edge to recognize brightness or slider adjustment drags.
 *
 * - Touch down inside region → Possible (buffering begins)
 * - Drag beyond threshold → Recognized (onRecognized + onMoved fire)
 * - Touch up while Possible → Failed (was a tap, not a drag)
 * - Long press while Possible → Failed (held still, not a drag)
 */

#pragma once

#include "GestureRecognizer.hpp"

/// @ingroup core
class EdgeDragGestureRecognizer final : public GestureRecognizer {
public:
    /// @brief Construct a recognizer for drags starting in a specific region.
    /// @param activationRegion Screen region (in window coordinates) where touches activate this recognizer.
    /// @param moveThreshold Minimum distance in pixels before the drag is recognized.
    EdgeDragGestureRecognizer(Rect activationRegion, int moveThreshold = 15);

    bool wantsTouch(Point windowPoint) override;
    void touchDown(Event event) override;
    void touchMoved(Event event) override;
    void touchUp(Event event) override;
    void longPress(Event event) override;
    void reset() override;

    /// @brief Get the initial touch-down point for this gesture.
    Point getTouchDownPoint() const { return this->touchDownPoint; }

private:
    Rect activationRegion;
    int moveThreshold;
    Point touchDownPoint;
};
