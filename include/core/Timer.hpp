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
 * @brief Timer class for scheduling callbacks at intervals or specific times.
 *
 * Timers run on a dedicated background thread and deliver callbacks on the
 * main run loop, ensuring thread safety for UI updates. Inspired by NSTimer.
 */

#pragma once

#include <memory>
#include <functional>
#include <chrono>
#include <cstdint>

/**
 * @brief A timer that fires a callback after an interval or at a specific time.
 *
 * Timers are managed by a global timer thread that sleeps until the next
 * scheduled fire time. When a timer fires, its callback is queued for
 * execution on the main thread (during the Application's run loop).
 *
 * Timers must be explicitly invalidated or will continue to fire (if repeating)
 * until invalidated. One-shot timers automatically invalidate after firing.
 *
 * Usage:
 * @code
 * // Fire once after 5 seconds
 * auto timer = Timer::scheduledTimer(5000, []() {
 *     printf("Timer fired!\n");
 * });
 *
 * // Fire every minute, starting at the top of the next minute
 * auto clockTimer = Timer::scheduledTimerForNextMinute([]() {
 *     updateClockDisplay();
 * }, true);
 *
 * // Cancel a timer
 * timer->invalidate();
 * @endcode
 */
class Timer : public std::enable_shared_from_this<Timer> {
public:
    /// @brief Clock type used for scheduling (system wall clock for absolute times).
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    /**
     * @brief Create and schedule a timer that fires after an interval.
     *
     * @param intervalMs Time in milliseconds until the timer fires.
     * @param callback Function to call when the timer fires.
     * @param repeats If true, the timer reschedules itself after firing.
     * @return A shared_ptr to the new timer. Keep this reference to invalidate later.
     */
    static std::shared_ptr<Timer> scheduledTimer(
        int64_t intervalMs,
        std::function<void()> callback,
        bool repeats = false
    );

    /**
     * @brief Create and schedule a timer that fires at a specific time.
     *
     * @param fireDate The absolute time when the timer should first fire.
     * @param intervalMs Interval for repeating (only used if repeats is true).
     * @param callback Function to call when the timer fires.
     * @param repeats If true, the timer repeats at intervalMs after the initial fire.
     * @return A shared_ptr to the new timer.
     */
    static std::shared_ptr<Timer> scheduledTimer(
        TimePoint fireDate,
        int64_t intervalMs,
        std::function<void()> callback,
        bool repeats = false
    );

    /**
     * @brief Create a timer that fires at the top of the next minute.
     *
     * Useful for clock displays that update on minute boundaries.
     *
     * @param callback Function to call when the timer fires.
     * @param repeats If true, fires every minute thereafter.
     * @return A shared_ptr to the new timer.
     */
    static std::shared_ptr<Timer> scheduledTimerForNextMinute(
        std::function<void()> callback,
        bool repeats = false
    );

    /**
     * @brief Create a timer that fires at the top of the next hour.
     *
     * @param callback Function to call when the timer fires.
     * @param repeats If true, fires every hour thereafter.
     * @return A shared_ptr to the new timer.
     */
    static std::shared_ptr<Timer> scheduledTimerForNextHour(
        std::function<void()> callback,
        bool repeats = false
    );

    /**
     * @brief Stop the timer from firing.
     *
     * After invalidation, the timer will not fire again. This is safe to call
     * multiple times or on an already-invalidated timer.
     */
    void invalidate();

    /**
     * @brief Check if the timer is still active.
     * @return true if the timer may still fire, false if invalidated.
     */
    bool isValid() const;

    /**
     * @brief Get the next scheduled fire time.
     * @return The time point when this timer will next fire.
     */
    TimePoint getFireDate() const { return fireDate; }

    /**
     * @brief Drain pending timer callbacks on the main thread.
     *
     * Called by Application::run() each iteration to execute any timer
     * callbacks that have been queued by the timer thread. This ensures
     * all callbacks run on the main thread.
     */
    static void drainPendingCallbacks();

    /**
     * @brief Initialize the timer subsystem.
     *
     * Starts the background timer thread. Called automatically when the
     * first timer is scheduled, but can be called explicitly at startup.
     */
    static void initialize();

    /**
     * @brief Shut down the timer subsystem.
     *
     * Stops the background timer thread and invalidates all pending timers.
     * Typically called at application exit.
     */
    static void shutdown();

    /**
     * @brief Get the number of currently scheduled timers.
     * @return The count of timers waiting to fire.
     */
    static size_t getScheduledCount();

private:
    Timer(TimePoint fireDate, int64_t intervalMs, std::function<void()> callback, bool repeats);

    TimePoint fireDate;
    int64_t intervalMs;
    std::function<void()> callback;
    bool repeats;
    bool valid = true;

    // Add this timer to the global schedule
    void schedule();

    // Internal: called by the timer subsystem when this timer fires
    // Public access needed for the timer thread's lambda, but not part of the API
    friend void fireTimer(std::shared_ptr<Timer> timer);
};
