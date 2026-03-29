/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
 */

#pragma once

#include "Focus.hpp"
#include "RelativeTracker.hpp"

/**
 * @brief Trackpad-style cursor position tracker.
 *
 * CursorManager lives on the Window and translates touch events into
 * cursor position updates. It does not render a cursor — views create
 * their own visual representation and reposition it via the onCursorMoved
 * callback.
 *
 * Processes TOUCH_DOWN, TOUCH_MOVED, and TOUCH_UP events to track
 * relative motion with configurable gain. Fires callbacks for position
 * changes, tracking start, and tracking end.
 */
class CursorManager {
public:
    /// Configure tracking gain for both axes (default 1.0).
    void setGain(float gain);

    /// Current cursor position in pixel coordinates.
    void getPosition(int &px, int &py) const;

    /// Set the cursor position directly (e.g., to sync after view switch).
    void setPosition(int px, int py);

    /// Callback fired when cursor position changes due to touch input.
    std::function<void(int px, int py)> onCursorMoved;

    /// Callback fired on touch-down (cursor tracking begins).
    std::function<void()> onTrackingBegan;

    /// Callback fired on touch-up (cursor tracking ends).
    std::function<void()> onTrackingEnded;

    /// Process a touch event. Called from Application::generateEvent()
    /// before gesture recognition and view dispatch.
    void handleTouchEvent(Event event, int maxX, int maxY);

private:
    RelativeTracker trackX{0, 1.0f};
    RelativeTracker trackY{0, 1.0f};
};
