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
 * @file ArabicShaping.hpp
 * @brief Arabic contextual shaping for connected script rendering.
 *
 * Replaces base Arabic characters (U+0621-U+06D2) with their contextual
 * presentation forms (isolated, initial, medial, final) based on neighboring
 * characters. Also handles Lam-Alef ligatures.
 */

#pragma once

#include <cstddef>
#include "Utf8.hpp"
#include "focus_config.h"

namespace focus {

#if FOCUS_HAS_TEXT_SHAPING

/**
 * @brief Shape Arabic text by replacing base characters with contextual forms.
 *
 * Modifies the codepoints array in place. Non-Arabic characters are left
 * unchanged. Characters consumed by Lam-Alef ligatures are replaced with
 * U+200B (Zero Width Space).
 *
 * @param codepoints Array of Unicode codepoints to shape.
 * @param len Number of codepoints in the array.
 */
void shapeArabic(UNICODE_CODEPOINT* codepoints, size_t len);

/// Shape Arabic text in place if any codepoint is in U+0621-U+06D2.
/// Idempotent: shaped output lies outside the detection range, so calling
/// this on already-shaped text leaves the buffer unchanged.
void shapeArabicIfNeeded(UNICODE_CODEPOINT* codepoints, size_t len);

#else

// Shaping compiled out: both entry points are inert.
inline void shapeArabic(UNICODE_CODEPOINT*, size_t) {}
inline void shapeArabicIfNeeded(UNICODE_CODEPOINT*, size_t) {}

#endif  // FOCUS_HAS_TEXT_SHAPING

}  // namespace focus
