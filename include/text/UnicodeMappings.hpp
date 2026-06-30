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
 * @file UnicodeMappings.hpp
 * @brief Unicode case conversion and mirroring functions.
 *
 * Provides O(log n) lookup for case conversion (uppercase, lowercase, titlecase)
 * and bidirectional mirroring mappings. Tables are generated from UnicodeData.txt
 * and BidiMirroring.txt.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include "utf8_decode.hpp"

namespace focus {

namespace UnicodeMappings {

/**
 * @brief Convert a codepoint to uppercase.
 * @param cp The input codepoint.
 * @return The uppercase equivalent, or the original if no mapping exists.
 */
UNICODE_CODEPOINT toUppercase(UNICODE_CODEPOINT cp);

/**
 * @brief Convert a codepoint to lowercase.
 * @param cp The input codepoint.
 * @return The lowercase equivalent, or the original if no mapping exists.
 */
UNICODE_CODEPOINT toLowercase(UNICODE_CODEPOINT cp);

/**
 * @brief Convert a codepoint to titlecase.
 * @param cp The input codepoint.
 * @return The titlecase equivalent, or the original if no mapping exists.
 */
UNICODE_CODEPOINT toTitlecase(UNICODE_CODEPOINT cp);

/**
 * @brief Get the bidirectional mirror of a codepoint.
 * @param cp The input codepoint (e.g., '(' or '[').
 * @return The mirrored codepoint (e.g., ')' or ']'), or the original if none.
 */
UNICODE_CODEPOINT toMirror(UNICODE_CODEPOINT cp);

/**
 * @brief Convert a buffer of codepoints to uppercase in place.
 * @param buf Buffer of codepoints to convert.
 * @param len Number of codepoints in the buffer.
 */
void toUppercase(UNICODE_CODEPOINT* buf, size_t len);

/**
 * @brief Convert a buffer of codepoints to lowercase in place.
 * @param buf Buffer of codepoints to convert.
 * @param len Number of codepoints in the buffer.
 */
void toLowercase(UNICODE_CODEPOINT* buf, size_t len);

} // namespace UnicodeMappings

}  // namespace focus
