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

#include <stdint.h>

/// @brief Factory class for named color constants and default color management.
///
/// Color provides static methods that return uint16_t color values by name,
/// replacing the duplicate getBlackColor()/getWhiteColor()/etc. getters that
/// previously existed on both CanvasView and Display.
///
/// It also manages the default foreground and background colors used by newly
/// created Views (previously managed by static members on View).
struct Color {
    static constexpr uint16_t Black()     { return 0; }
    static constexpr uint16_t DarkGray()  { return 1; }
    static constexpr uint16_t LightGray() { return 2; }
    static constexpr uint16_t White()     { return 3; }

    static uint16_t DefaultForegroundColor();
    static uint16_t DefaultBackgroundColor();
    static void SetDefaultForegroundColor(uint16_t color);
    static void SetDefaultBackgroundColor(uint16_t color);
};
