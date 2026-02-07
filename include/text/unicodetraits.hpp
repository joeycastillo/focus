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
 * @file unicodetraits.hpp
 * @brief Unicode character property lookup for text layout and bidirectional support.
 *
 * Provides a compact bitfield of Unicode properties for any codepoint, including
 * whitespace classification, line break opportunities, directional affinity (LTR/RTL),
 * and combining mark detection. Used by the text layout engine for word wrapping
 * and by renderers for bidirectional text handling.
 */

#pragma once

#include <stdint.h>
#include "utf8_decode.hpp"

/**
 * @brief Packed bitfield of Unicode character properties.
 *
 * Can be accessed either as individual bit flags via the `is` struct, or
 * as a single packed byte for efficient comparison.
 */
typedef union {
    struct {
        uint8_t controlchar: 1; ///< Is a control character (U+0000..U+001F, etc.).
        uint8_t whitespace: 1;  ///< Is a whitespace character (space, tab, etc.).
        uint8_t linebreak: 1;   ///< A line break opportunity exists after this character.
        uint8_t nsm: 1;         ///< Is a nonspacing mark (combining diacritical, etc.).
        uint8_t rtl: 1;         ///< Has strong right-to-left directionality (Arabic, Hebrew, etc.).
        uint8_t ltr: 1;         ///< Has strong left-to-right directionality.
        uint8_t mirrored: 1;    ///< Should be drawn mirrored in RTL text runs (parentheses, etc.).
        uint8_t mapped: 1;      ///< A mirror-image mapping exists for this character.
    } is;
    uint8_t packed; ///< All flags packed into a single byte.
} unicode_info_t;

/**
 * @brief Look up Unicode properties for a codepoint.
 * @param codepoint The Unicode codepoint to query.
 * @return A unicode_info_t with the relevant property flags set.
 */
unicode_info_t getTraitsForCodepoint(UNICODE_CODEPOINT codepoint);
