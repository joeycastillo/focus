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
 * @file DeferredTask.hpp
 * @brief A one-shot task that executes a callback after a delay.
 */

#pragma once

#include "Task.hpp"
#include <functional>

namespace focus {

/// A one-shot task that executes a callback after a specified number of run loop cycles.
/// After executing, it removes itself from the application's task list.
///
/// Usage:
///   auto task = std::make_shared<DeferredTask>([this]() {
///       // Code to run after delay
///   }, 1);  // Wait 1 cycle before executing
///   application->addTask(task);
/// @ingroup core
class DeferredTask final : public Task {
public:
    /// Create a deferred task.
    /// @param callback The function to execute
    /// @param delayCycles Number of run loop cycles to wait before executing (default: 1)
    DeferredTask(std::function<void()> callback, int delayCycles = 1);

    /// Run the task. Returns true when the task should be removed.
    bool run(std::shared_ptr<Application> application) override;

private:
    std::function<void()> callback;
    int remainingCycles;
};

}  // namespace focus
