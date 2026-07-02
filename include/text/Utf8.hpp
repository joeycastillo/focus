/*
 * MIT License
 *
 * Copyright (c) 2022-2026 Joey Castillo
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
 * @file Utf8.hpp
 * @brief UTF-8 decoding: the codepoint type, a streaming decoder, and parse helpers.
 *
 * All decoding is strictly validating: overlong encodings, UTF-16 surrogates
 * (U+D800 through U+DFFF), codepoints above U+10FFFF, and stray or truncated
 * continuation bytes are rejected.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef UNICODE_BMP_ONLY
#define UNICODE_CODEPOINT uint16_t
#define UTF8_END   0xFFFF   ///< U+FFFF is a Unicode noncharacter; safe as BMP sentinel
#define UTF8_ERROR 0xFFFE   ///< U+FFFE is a Unicode noncharacter; safe as BMP sentinel
#else
#define UNICODE_CODEPOINT uint32_t
#define UTF8_END   0x110000  ///< First value above legal Unicode range; unambiguous sentinel
#define UTF8_ERROR 0x110001
#endif

#define UTF8_REPLACEMENT_CHARACTER 0xFFFD  ///< U+FFFD: a real codepoint, not a sentinel

/// utf8DecodeStep state: a complete codepoint has been decoded.
#define UTF8_ACCEPT 0
/// utf8DecodeStep state: the byte sequence is invalid.
#define UTF8_REJECT 12

namespace focus {

/**
 * @brief Feed one byte to the UTF-8 decoding automaton.
 *
 * Streaming core of the decoder (Bjoern Hoehrmann's DFA — see Utf8.cpp for
 * attribution). The caller owns the automaton state, so any number of decodes
 * can be in flight at once.
 *
 * @param state In/out automaton state. Initialize to UTF8_ACCEPT. After the
 *              call: UTF8_ACCEPT means @p codepoint holds a complete codepoint,
 *              UTF8_REJECT means the input is invalid, and any other value
 *              means more bytes are needed.
 * @param codepoint In/out partial codepoint, valid when state is UTF8_ACCEPT.
 * @param byte The next byte of input.
 * @return The new state (same value stored to @p state).
 */
uint32_t utf8DecodeStep(uint32_t& state, uint32_t& codepoint, uint8_t byte);

/**
 * @brief Decode the next codepoint from a UTF-8 buffer and advance the cursor.
 *
 * @param cursor In/out position in the buffer. Advanced past the decoded
 *               codepoint on success; left unchanged on error.
 * @param end One past the last byte of the buffer.
 * @return The decoded codepoint, UTF8_END when the cursor reaches @p end, or
 *         UTF8_ERROR on invalid input (including a sequence truncated by the
 *         end of the buffer). When UNICODE_BMP_ONLY is defined, codepoints
 *         that don't fit the 16-bit type decode as UTF8_REPLACEMENT_CHARACTER.
 */
UNICODE_CODEPOINT utf8_next(const char*& cursor, const char* end);

/**
 * @brief Count the codepoints in a NUL-terminated UTF-8 string. O(n) time.
 * @param string The UTF-8 string.
 * @return The number of codepoints, or 0 if the string is empty or invalid.
 */
size_t utf8_codepoint_length(const char* string);

/**
 * @brief Parse a NUL-terminated UTF-8 string into codepoints. O(n) time.
 * @param string The UTF-8 string.
 * @param buf Receives the parsed codepoints; must have room for
 *            utf8_codepoint_length(string) entries. Pass NULL to count
 *            without writing.
 * @return The number of codepoints, or 0 if the string is empty or invalid.
 */
size_t utf8_parse(const char* string, UNICODE_CODEPOINT* buf);

}  // namespace focus
