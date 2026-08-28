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
 * @file Task.hpp
 * @brief Abstract base class for cooperatively scheduled tasks in the run loop.
 *
 * Tasks are the primary mechanism for non-UI work in Focus. The Application
 * runs all registered tasks once per iteration of its main loop. Tasks can
 * be long-lived (returning false to stay in the list) or one-shot (returning
 * true to remove themselves after execution).
 */

#pragma once

#include "Focus.hpp"

namespace focus {

/**
 * @brief Abstract base class for cooperatively scheduled tasks.
 *
 * Subclass Task and implement run() to perform periodic work such as
 * polling input devices, refreshing the display, or running background
 * computations.
 * @ingroup core
 */
class Task {
public:
    Task();
    virtual ~Task() = default;

    /**
     * @brief Execute one iteration of this task.
     *
     * Called once per main loop iteration by the Application.
     *
     * @param application The owning application.
     * @return true to remove this task from the run loop, false to keep
     *         running on subsequent iterations. A long-running task returns
     *         false until its work is done, then true to remove itself.
     */
    virtual bool run(std::shared_ptr<Application> application) = 0;

    /**
     * @brief Whether this task has work that needs the run loop at full speed.
     *
     * By default the run loop calls every task as fast as it can, forever. On
     * real hardware that's wasteful: when nothing is happening, an application
     * may want to stretch out iterations — yield to the host OS, poll at a
     * slower cadence, or enter a platform-specific sleep state between passes.
     * Focus is platform-agnostic and does none of this itself, but an app can
     * implement such a pacing policy (perhaps in a task!), and that task needs
     * to know whether slowing the loop down would stall work in progress.
     *
     * isBusy is our answer to this. A Task should return true while it's doing
     * work that only advances when run() is called, often incremental progress
     * that it wants to do across run loop invocations without blocking the UI.
     * Return false when slower iterations are fine, even for a task that does
     * something every time it runs.
     *
     * Waiting on an outside event doesn't count as busy, but give a thought to
     * how the event gets noticed. On hardware where a touch or a button press
     * fires an interrupt that wakes the loop, an input task holds no work until
     * the event arrives. On hardware that has to poll to catch the event, the
     * answer is still not to claim busy — a bool can't say how slow is too
     * slow, only "never slow down," which would keep the loop at full speed
     * forever. A board like that needs a floor on how slow the loop may go
     * while idle, and the floor belongs to the pacing policy. Busy describes
     * the work a task already has, not the work that might show up.
     */
    virtual bool isBusy() const { return false; }
};

}  // namespace focus

