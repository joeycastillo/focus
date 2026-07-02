/*
 * Internal platform abstraction for Focus framework.
 * Not part of the public API — use for Focus source files only.
 */
#pragma once

/**
 * @brief Monotonic microseconds since an arbitrary epoch.
 *
 * Focus uses this clock to timestamp touch events and classify gestures
 * (tap vs. swipe vs. long press), so it must be monotonic and actually
 * advance — a stub implementation would silently break gesture timing.
 * On ESP-IDF this is esp_timer; elsewhere std::chrono::steady_clock.
 * Ports to targets without either supply their own branch here.
 */
#ifdef ESP_PLATFORM
#include "esp_timer.h"
static inline int64_t focus_timer_get_time() {
    return esp_timer_get_time();
}
#else
#include <chrono>
static inline int64_t focus_timer_get_time() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}
#endif
