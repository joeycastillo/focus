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

#include "Timer.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <algorithm>
#include <atomic>

// Global state for the timer subsystem
namespace {
    // The timer thread
    std::thread timerThread;
    std::atomic<bool> timerThreadRunning{false};
    std::atomic<bool> timerThreadShouldStop{false};

    // Mutex protecting the timer list and condition variable
    std::mutex timerMutex;
    std::condition_variable timerCV;

    // All scheduled timers, sorted by fire date (soonest first)
    std::vector<std::shared_ptr<Timer>> scheduledTimers;

    // Callbacks queued to run on the main thread
    std::mutex callbackMutex;
    std::vector<std::function<void()>> pendingCallbacks;

    // Compare timers by fire date for sorting
    bool timerComparator(const std::shared_ptr<Timer>& a, const std::shared_ptr<Timer>& b) {
        return a->getFireDate() > b->getFireDate(); // > for min-heap behavior with pop_back
    }
}

// Friend function to fire a timer (allows access to private members)
void fireTimer(std::shared_ptr<Timer> timer) {
    if (!timer->valid) {
        return;
    }

    // Execute the callback
    if (timer->callback) {
        timer->callback();
    }

    // Reschedule if repeating
    if (timer->repeats && timer->valid) {
        timer->fireDate = Timer::Clock::now() + std::chrono::milliseconds(timer->intervalMs);
        timer->schedule();
    } else {
        timer->valid = false;
    }
}

namespace {

    void timerThreadFunc() {
        std::unique_lock<std::mutex> lock(timerMutex);

        while (!timerThreadShouldStop.load()) {
            if (scheduledTimers.empty()) {
                // No timers scheduled, wait indefinitely until one is added
                timerCV.wait(lock, []() {
                    return !scheduledTimers.empty() || timerThreadShouldStop.load();
                });
                continue;
            }

            // Get the next timer to fire (last element due to reverse sort)
            auto nextTimer = scheduledTimers.back();
            auto now = Timer::Clock::now();

            if (nextTimer->getFireDate() <= now) {
                // Timer is ready to fire
                scheduledTimers.pop_back();

                if (nextTimer->isValid()) {
                    // Queue the callback for main thread execution
                    {
                        std::lock_guard<std::mutex> cbLock(callbackMutex);
                        auto timerPtr = nextTimer;
                        pendingCallbacks.push_back([timerPtr]() {
                            fireTimer(timerPtr);
                        });
                    }
                }
            } else {
                // Wait until the next timer should fire
                timerCV.wait_until(lock, nextTimer->getFireDate(), []() {
                    return timerThreadShouldStop.load();
                });
            }
        }
    }
}

Timer::Timer(TimePoint fireDate, int64_t intervalMs, std::function<void()> callback, bool repeats)
    : fireDate(fireDate)
    , intervalMs(intervalMs)
    , callback(std::move(callback))
    , repeats(repeats)
    , valid(true) {
}

void Timer::initialize() {
    if (timerThreadRunning.load()) {
        return;
    }

    timerThreadShouldStop.store(false);
    timerThread = std::thread(timerThreadFunc);
    timerThreadRunning.store(true);
}

void Timer::shutdown() {
    if (!timerThreadRunning.load()) {
        return;
    }

    // Signal the thread to stop
    timerThreadShouldStop.store(true);
    timerCV.notify_all();

    // Wait for the thread to finish
    if (timerThread.joinable()) {
        timerThread.join();
    }

    timerThreadRunning.store(false);

    // Clear all pending timers and callbacks
    {
        std::lock_guard<std::mutex> lock(timerMutex);
        scheduledTimers.clear();
    }
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        pendingCallbacks.clear();
    }
}

void Timer::drainPendingCallbacks() {
    std::vector<std::function<void()>> callbacks;

    // Quickly swap out the pending callbacks
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbacks.swap(pendingCallbacks);
    }

    // Execute all callbacks on the main thread
    for (auto& cb : callbacks) {
        cb();
    }
}

void Timer::schedule() {
    // Ensure the timer thread is running
    initialize();

    std::lock_guard<std::mutex> lock(timerMutex);

    // Add to the list and re-sort
    scheduledTimers.push_back(shared_from_this());
    std::sort(scheduledTimers.begin(), scheduledTimers.end(), timerComparator);

    // Wake the timer thread in case this timer fires sooner than what it was waiting for
    timerCV.notify_one();
}

void Timer::invalidate() {
    valid = false;

    // Remove from scheduled timers
    std::lock_guard<std::mutex> lock(timerMutex);
    auto it = std::find_if(scheduledTimers.begin(), scheduledTimers.end(),
        [this](const std::shared_ptr<Timer>& t) { return t.get() == this; });
    if (it != scheduledTimers.end()) {
        scheduledTimers.erase(it);
    }
}

bool Timer::isValid() const {
    return valid;
}

std::shared_ptr<Timer> Timer::scheduledTimer(
    int64_t intervalMs,
    std::function<void()> callback,
    bool repeats
) {
    auto fireDate = Clock::now() + std::chrono::milliseconds(intervalMs);
    auto timer = std::shared_ptr<Timer>(new Timer(fireDate, intervalMs, std::move(callback), repeats));
    timer->schedule();
    return timer;
}

std::shared_ptr<Timer> Timer::scheduledTimer(
    TimePoint fireDate,
    int64_t intervalMs,
    std::function<void()> callback,
    bool repeats
) {
    auto timer = std::shared_ptr<Timer>(new Timer(fireDate, intervalMs, std::move(callback), repeats));
    timer->schedule();
    return timer;
}

std::shared_ptr<Timer> Timer::scheduledTimerForNextMinute(
    std::function<void()> callback,
    bool repeats
) {
    // Get current time
    auto now = Clock::now();
    auto nowTime = Clock::to_time_t(now);
    struct tm* tm = localtime(&nowTime);

    // Calculate seconds until the next minute
    int secondsUntilNextMinute = 60 - tm->tm_sec;
    auto fireDate = now + std::chrono::seconds(secondsUntilNextMinute);

    // For repeating, interval is 60 seconds
    int64_t intervalMs = 60 * 1000;

    auto timer = std::shared_ptr<Timer>(new Timer(fireDate, intervalMs, std::move(callback), repeats));
    timer->schedule();
    return timer;
}

std::shared_ptr<Timer> Timer::scheduledTimerForNextHour(
    std::function<void()> callback,
    bool repeats
) {
    // Get current time
    auto now = Clock::now();
    auto nowTime = Clock::to_time_t(now);
    struct tm* tm = localtime(&nowTime);

    // Calculate seconds until the next hour
    int secondsUntilNextHour = (60 - tm->tm_min) * 60 - tm->tm_sec;
    if (secondsUntilNextHour <= 0) {
        secondsUntilNextHour += 3600;
    }
    auto fireDate = now + std::chrono::seconds(secondsUntilNextHour);

    // For repeating, interval is 1 hour
    int64_t intervalMs = 60 * 60 * 1000;

    auto timer = std::shared_ptr<Timer>(new Timer(fireDate, intervalMs, std::move(callback), repeats));
    timer->schedule();
    return timer;
}
