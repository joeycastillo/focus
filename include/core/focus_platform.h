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
 * On ESP-IDF this is esp_timer; on Arduino cores, micros(); elsewhere
 * std::chrono::steady_clock. Ports to targets without any of these
 * supply their own branch here.
 */
#if defined(ESP_PLATFORM)
#include "esp_timer.h"
static inline int64_t focus_timer_get_time() {
    return esp_timer_get_time();
}
#elif defined(ARDUINO)
#include <Arduino.h>
static inline int64_t focus_timer_get_time() {
    // micros() wraps every ~71.6 minutes (uint32_t). Focus only takes
    // short deltas from this clock (gesture classification, timers,
    // animation stepping), so the wrap's worst case is one timer or
    // gesture misbehaving at the wrap instant — acceptable here.
    return (int64_t)micros();
}
#elif defined(FOCUS_PLATFORM_PICO)
#include "pico/time.h"
static inline int64_t focus_timer_get_time() {
    return (int64_t)time_us_64();
}
#else
#include <chrono>
static inline int64_t focus_timer_get_time() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}
#endif
