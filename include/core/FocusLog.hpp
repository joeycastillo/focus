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

#pragma once

// Focus-native logging macros. On ESP-IDF, delegate to its logging system.
// On other platforms, E/W/I go to stderr (matching ESP-IDF's default INFO
// threshold) and D/V are compiled out but still type-checked.

#ifdef ESP_PLATFORM
#include "esp_log.h"
#define FOCUS_LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#define FOCUS_LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define FOCUS_LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define FOCUS_LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define FOCUS_LOGV(tag, format, ...) ESP_LOGV(tag, format, ##__VA_ARGS__)
#else
#include <cstdio>
#define FOCUS_LOGE(tag, format, ...) fprintf(stderr, "E (%s) " format "\n", tag, ##__VA_ARGS__)
#define FOCUS_LOGW(tag, format, ...) fprintf(stderr, "W (%s) " format "\n", tag, ##__VA_ARGS__)
#define FOCUS_LOGI(tag, format, ...) fprintf(stderr, "I (%s) " format "\n", tag, ##__VA_ARGS__)
#define FOCUS_LOGD(tag, format, ...) do { if (0) fprintf(stderr, "D (%s) " format "\n", tag, ##__VA_ARGS__); } while(0)
#define FOCUS_LOGV(tag, format, ...) do { if (0) fprintf(stderr, "V (%s) " format "\n", tag, ##__VA_ARGS__); } while(0)
#endif
