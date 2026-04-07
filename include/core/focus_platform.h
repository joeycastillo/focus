/*
 * Internal platform abstraction for Focus framework.
 * Not part of the public API — use for Focus source files only.
 */
#pragma once

#ifdef ESP_PLATFORM
#include "esp_timer.h"
#else
#include <chrono>
static inline int64_t esp_timer_get_time() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}
#endif
