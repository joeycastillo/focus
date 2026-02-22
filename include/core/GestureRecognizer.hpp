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
 * @file GestureRecognizer.hpp
 * @brief Abstract base class for system-level gesture recognizers.
 *
 * System gesture recognizers are attached to a Window and get first crack at
 * touch events before they are dispatched to the normal view hierarchy. They
 * are intended for continuous gestures (like edge drags) that need to intercept
 * and consume an entire touch sequence.
 *
 * While a recognizer is in the Possible state, the Application buffers touch
 * events. If the recognizer transitions to Recognized, the buffered events are
 * discarded and subsequent events go to the recognizer's callbacks. If the
 * recognizer transitions to Failed, the buffered events are replayed through
 * normal view dispatch.
 */

#pragma once

#include "Focus.hpp"
#include <functional>

class GestureRecognizer {
public:
    /// @brief Possible states for a gesture recognizer.
    enum class State {
        Possible,    ///< Touch sequence started but gesture not yet determined.
        Recognized,  ///< Gesture matched; recognizer owns the touch sequence.
        Failed       ///< Gesture did not match; touch should go to normal dispatch.
    };

    virtual ~GestureRecognizer() = default;

    /// @brief Whether this recognizer is interested in a touch starting at this point.
    /// Returning false skips this recognizer entirely (no buffering needed).
    virtual bool wantsTouch(Point windowPoint) = 0;

    /// @brief Called when a touch sequence begins.
    virtual void touchDown(Event event) = 0;

    /// @brief Called when the touch moves.
    virtual void touchMoved(Event event) = 0;

    /// @brief Called when the touch ends.
    virtual void touchUp(Event event) = 0;

    /// @brief Called when a long press is detected during the touch.
    virtual void longPress(Event event) = 0;

    /// @brief Reset the recognizer to its initial state for a new touch sequence.
    virtual void reset() = 0;

    /// @brief Get the current recognition state.
    State getState() const { return this->state; }

    /// @brief Called once when the recognizer transitions from Possible to Recognized.
    std::function<void(Event)> onRecognized;

    /// @brief Called on each TOUCH_MOVED after recognition.
    std::function<void(Event)> onMoved;

    /// @brief Called on TOUCH_UP after recognition (gesture complete).
    std::function<void(Event)> onEnded;

protected:
    State state = State::Possible;
};
