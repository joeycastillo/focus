/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
 */

#include "AnimationSequence.hpp"
#include "View.hpp"

AnimationSequence &AnimationSequence::animate(
    std::chrono::milliseconds interval,
    int frameCount,
    std::function<void(int frame)> onFrame)
{
    steps_.push_back({StepType::Animate, interval, frameCount,
                      std::move(onFrame), nullptr});
    return *this;
}

AnimationSequence &AnimationSequence::then(std::function<void()> action)
{
    steps_.push_back({StepType::Action, {}, 0, nullptr, std::move(action)});
    return *this;
}

AnimationSequence &AnimationSequence::delay(std::chrono::milliseconds duration)
{
    steps_.push_back({StepType::Delay, duration, 0, nullptr, nullptr});
    return *this;
}

void AnimationSequence::start(std::shared_ptr<View> owner, std::function<void()> onComplete)
{
    cancel();
    owner_ = owner;
    onComplete_ = std::move(onComplete);
    currentStep_ = 0;
    running_ = true;
    advanceToStep(0);
}

void AnimationSequence::cancel()
{
    if (timer_) {
        timer_->invalidate();
        timer_.reset();
    }
    running_ = false;
    owner_.reset();
    onComplete_ = nullptr;
}

void AnimationSequence::advanceToStep(size_t index)
{
    if (!running_) return;

    // Process immediate (Action) steps synchronously until we hit a
    // timed step or run out of steps.
    while (index < steps_.size()) {
        currentStep_ = index;
        auto &step = steps_[index];

        switch (step.type) {
        case StepType::Action:
            if (step.action) step.action();
            index++;
            continue;

        case StepType::Delay: {
            auto owner = owner_.lock();
            if (!owner) { finish(); return; }
            timer_ = owner->scheduledTimer(step.interval,
                [this, next = index + 1](Timer &) {
                    timer_.reset();
                    advanceToStep(next);
                });
            if (!timer_) { finish(); return; }
            return;
        }

        case StepType::Animate: {
            auto owner = owner_.lock();
            if (!owner || step.frameCount <= 0) {
                index++;
                continue;
            }
            currentFrame_ = 0;
            if (step.onFrame) step.onFrame(0);
            if (step.frameCount == 1) {
                index++;
                continue;
            }
            timer_ = owner->scheduledTimer(step.interval,
                [this, next = index + 1](Timer &timer) {
                    currentFrame_++;
                    auto &s = steps_[currentStep_];
                    if (currentFrame_ >= s.frameCount) {
                        timer.invalidate();
                        timer_.reset();
                        advanceToStep(next);
                    } else {
                        if (s.onFrame) s.onFrame(currentFrame_);
                    }
                },
                true);
            if (!timer_) { finish(); return; }
            return;
        }
        }
    }

    // All steps done
    finish();
}

void AnimationSequence::finish()
{
    running_ = false;
    auto cb = std::move(onComplete_);
    owner_.reset();
    if (cb) cb();
}
