#pragma once
#include <cmath>

namespace focus {

/**
 * Trackpad-style relative position tracker.
 * On touch-down call anchor(), on touch-moved call track().
 * Both raw input and position are in pixel space by default (gain=1.0).
 * `gain` is a multiplier on the raw delta.
 */
struct RelativeTracker {
    float position;
    float gain;

    RelativeTracker(float initialPosition = 0, float gain = 1.0f)
        : position(initialPosition), gain(gain) {}

    void anchor(float rawPosition) {
        rawAnchor = rawPosition;
        posAnchor = position;
    }

    float track(float rawPosition, float lo, float hi) {
        position = posAnchor + (rawPosition - rawAnchor) * gain;
        if (position < lo) position = lo;
        if (position > hi) position = hi;
        return position;
    }

private:
    float rawAnchor = 0;
    float posAnchor = 0;
};

}  // namespace focus
