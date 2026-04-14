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
 * @file Timer.hpp
 * @brief Cooperative timer that fires callbacks from the main run loop.
 *
 * Timer is a Task subclass that checks wall-clock time on each run loop
 * iteration and fires a callback when the scheduled time arrives. All
 * callbacks execute on the main thread — no background threads, no
 * synchronization, no global state.
 *
 * Timers are owned by the Application's task list. When the Application
 * exits its run loop (or an ELF app returns from run()), all timers are
 * naturally cleaned up.
 */

#pragma once

#include "Task.hpp"
#include <functional>
#include <chrono>
#include <ctime>

/**
 * @brief A cooperative timer that fires from the main run loop.
 *
 * Usage:
 * @code
 * // Fire once after 5 seconds
 * auto timer = Timer::scheduledTimer(app, std::chrono::seconds(5),
 *     [](Timer&) { printf("Timer fired!\n"); });
 *
 * // Fire every 500ms, stop when done
 * auto poller = Timer::scheduledTimer(app, std::chrono::milliseconds(500),
 *     [this](Timer& timer) {
 *         if (downloadFinished) {
 *             timer.invalidate();
 *             return;
 *         }
 *         updateProgressBar();
 *     }, true);
 *
 * // Update a clock display at each minute boundary
 * auto clock = Timer::scheduledTimerForNextMinute(app,
 *     [this](Timer&) { updateTimeDisplay(); }, true);
 * @endcode
 * @ingroup core
 */
class Timer final : public Task {
public:
    using Clock = std::chrono::steady_clock;

    /// @brief Callback type for timer events.
    ///
    /// The Timer reference is valid only for the duration of the callback.
    /// Do not store it — once a timer is invalidated and removed from the
    /// task list, any stored pointer or reference is undefined behavior.
    using Callback = std::function<void(Timer&)>;

    /**
     * @brief Create and schedule a timer that fires after an interval.
     *
     * @param application The application whose task list will own this timer.
     * @param interval Time until the timer fires (and repeat interval if repeating).
     * @param callback Function to call when the timer fires.
     * @param repeats If true, the timer reschedules itself after each fire.
     * @return A shared_ptr to the timer. Keep this to invalidate() later if needed.
     */
    static std::shared_ptr<Timer> scheduledTimer(
        std::shared_ptr<Application> application,
        std::chrono::milliseconds interval,
        Callback callback,
        bool repeats = false);

    /**
     * @brief Create a timer that fires at the top of the next minute.
     *
     * If repeating, recomputes the next minute boundary after each fire
     * using the system clock, so it self-corrects after NTP adjustments
     * rather than accumulating drift.
     *
     * @param application The application whose task list will own this timer.
     * @param callback Function to call when the timer fires.
     * @param repeats If true, fires at each subsequent minute boundary.
     * @return A shared_ptr to the timer.
     */
    static std::shared_ptr<Timer> scheduledTimerForNextMinute(
        std::shared_ptr<Application> application,
        Callback callback,
        bool repeats = false);

    /**
     * @brief Create a timer that fires at the top of the next hour.
     *
     * If repeating, recomputes the next hour boundary after each fire
     * using the system clock, so it self-corrects after NTP adjustments
     * rather than accumulating drift.
     *
     * @param application The application whose task list will own this timer.
     * @param callback Function to call when the timer fires.
     * @param repeats If true, fires at each subsequent hour boundary.
     * @return A shared_ptr to the timer.
     */
    static std::shared_ptr<Timer> scheduledTimerForNextHour(
        std::shared_ptr<Application> application,
        Callback callback,
        bool repeats = false);

    /**
     * @brief Restart the timer's interval from now.
     *
     * Resets the fire date to now + interval, so the full interval must
     * elapse before the next fire. Useful for "idle" timers that should
     * restart when activity occurs (e.g. cursor blink on movement).
     */
    void reset();

    /**
     * @brief Stop the timer from firing.
     *
     * The timer will be removed from the task list on the next run loop
     * iteration. Safe to call from within the timer's own callback, from
     * other code, or on an already-invalidated timer.
     */
    void invalidate();

    /**
     * @brief Check if the timer is still active.
     * @return true if the timer may still fire, false if invalidated.
     */
    bool isValid() const;

    /// @brief Task interface. Called by the run loop each iteration.
    bool run(std::shared_ptr<Application> application) override;

private:
    Timer(Clock::time_point fireDate,
          std::chrono::milliseconds interval,
          Callback callback,
          bool repeats,
          std::function<std::chrono::milliseconds()> recomputeInterval = nullptr);

    Clock::time_point fireDate;
    std::chrono::milliseconds interval;
    Callback callback;
    bool repeats;
    bool valid = true;

    /// For calendar-aligned timers: recomputes the interval to the next
    /// boundary using the system clock. Null for fixed-interval timers.
    std::function<std::chrono::milliseconds()> recomputeInterval;
};
