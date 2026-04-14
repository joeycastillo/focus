/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
 */

#pragma once

#include "Focus.hpp"
#include "Timer.hpp"
#include <chrono>
#include <vector>

class View;

/**
 * @brief A declarative sequence of animation steps and actions.
 *
 * AnimationSequence lets you chain timed frame-based animations,
 * immediate actions, and delays into a single readable pipeline.
 * Steps execute in order; each step completes before the next begins.
 *
 * Example:
 * @code
 *   auto seq = std::make_shared<AnimationSequence>();
 *   seq->animate(std::chrono::milliseconds(120), 5, [](int frame) {
 *           // called once per frame (0..4)
 *       })
 *      .then([] { ... })
 *      .delay(std::chrono::milliseconds(500))
 *      .then([] { ... });
 *   seq->start(myView, [] { ... });
 * @endcode
 *
 * The sequence creates timers through the provided View's scheduledTimer()
 * method, so the view must be in a window for the duration.
 *
 * Call cancel() to abort a running sequence. The completion callback
 * is NOT fired on cancellation.
 * @ingroup core
 */
class AnimationSequence {
public:
    /// Add a frame-based animation step. The callback fires once per frame
    /// (frame index 0 through frameCount-1) at the given interval.
    AnimationSequence &animate(std::chrono::milliseconds interval,
                               int frameCount,
                               std::function<void(int frame)> onFrame);

    /// Add an immediate action step (executes synchronously, then advances).
    AnimationSequence &then(std::function<void()> action);

    /// Add a delay step (waits for the given duration, then advances).
    AnimationSequence &delay(std::chrono::milliseconds duration);

    /// Begin executing the sequence. The view is used to create timers.
    /// The optional onComplete callback fires after the last step finishes.
    void start(std::shared_ptr<View> owner, std::function<void()> onComplete = nullptr);

    /// Cancel a running sequence. The completion callback is NOT fired.
    void cancel();

    /// Returns true if the sequence is currently executing.
    bool isRunning() const { return running_; }

private:
    enum class StepType { Animate, Action, Delay };

    struct Step {
        StepType type;
        std::chrono::milliseconds interval{0};
        int frameCount = 0;
        std::function<void(int)> onFrame;
        std::function<void()> action;
    };

    std::vector<Step> steps_;
    size_t currentStep_ = 0;
    int currentFrame_ = 0;
    bool running_ = false;
    std::weak_ptr<View> owner_;
    std::function<void()> onComplete_;
    std::shared_ptr<Timer> timer_;

    void advanceToStep(size_t index);
    void finish();
};
