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
#include "Application.hpp"

Timer::Timer(Clock::time_point fireDate,
             std::chrono::milliseconds interval,
             Callback callback,
             bool repeats,
             std::function<std::chrono::milliseconds()> recomputeInterval)
    : fireDate(fireDate)
    , interval(interval)
    , callback(std::move(callback))
    , repeats(repeats)
    , recomputeInterval(std::move(recomputeInterval)) {
}

bool Timer::run(std::shared_ptr<Application> application) {
    if (!valid) return true;

    if (Clock::now() < fireDate) return false;

    callback(*this);

    if (!repeats || !valid) return !repeats;

    if (recomputeInterval) {
        fireDate = Clock::now() + recomputeInterval();
    } else {
        fireDate += interval;
    }

    return false;
}

void Timer::reset() {
    if (valid) {
        fireDate = Clock::now() + interval;
    }
}

void Timer::invalidate() {
    valid = false;
}

bool Timer::isValid() const {
    return valid;
}

std::shared_ptr<Timer> Timer::scheduledTimer(
    std::shared_ptr<Application> application,
    std::chrono::milliseconds interval,
    Callback callback,
    bool repeats
) {
    auto fireDate = Clock::now() + interval;
    auto timer = std::shared_ptr<Timer>(
        new Timer(fireDate, interval, std::move(callback), repeats));
    application->addTask(timer);
    return timer;
}

static std::chrono::milliseconds millisecondsUntilNextMinute() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    return std::chrono::seconds(60 - t.tm_sec);
}

static std::chrono::milliseconds millisecondsUntilNextHour() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    int secondsUntil = (60 - t.tm_min) * 60 - t.tm_sec;
    if (secondsUntil <= 0) secondsUntil += 3600;
    return std::chrono::seconds(secondsUntil);
}

std::shared_ptr<Timer> Timer::scheduledTimerForNextMinute(
    std::shared_ptr<Application> application,
    Callback callback,
    bool repeats
) {
    auto initialInterval = millisecondsUntilNextMinute();
    auto fireDate = Clock::now() + initialInterval;

    std::function<std::chrono::milliseconds()> recompute = nullptr;
    if (repeats) {
        recompute = millisecondsUntilNextMinute;
    }

    auto timer = std::shared_ptr<Timer>(
        new Timer(fireDate, std::chrono::minutes(1), std::move(callback),
                  repeats, std::move(recompute)));
    application->addTask(timer);
    return timer;
}

std::shared_ptr<Timer> Timer::scheduledTimerForNextHour(
    std::shared_ptr<Application> application,
    Callback callback,
    bool repeats
) {
    auto initialInterval = millisecondsUntilNextHour();
    auto fireDate = Clock::now() + initialInterval;

    std::function<std::chrono::milliseconds()> recompute = nullptr;
    if (repeats) {
        recompute = millisecondsUntilNextHour;
    }

    auto timer = std::shared_ptr<Timer>(
        new Timer(fireDate, std::chrono::hours(1), std::move(callback),
                  repeats, std::move(recompute)));
    application->addTask(timer);
    return timer;
}
