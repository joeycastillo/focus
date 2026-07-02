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

#include "Utf8.hpp"
#include <cstring>

namespace focus {

/*
 * The lookup table and utf8DecodeStep are Bjoern Hoehrmann's DFA UTF-8 decoder.
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 *
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
static const uint8_t utf8d[] = {
    // The first part of the table maps bytes to character classes.
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 00..1f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 20..3f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 40..5f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 60..7f
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,  // 80..9f
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  // a0..bf
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  // c0..df
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8, // e0..ff
    // The second part maps a combination of automaton state and character
    // class to the next state. States are multiples of 12.
     0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12,
};

uint32_t utf8DecodeStep(uint32_t& state, uint32_t& codepoint, uint8_t byte) {
    uint32_t type = utf8d[byte];
    codepoint = (state != UTF8_ACCEPT)
        ? (byte & 0x3Fu) | (codepoint << 6)
        : (0xFFu >> type) & byte;
    state = utf8d[256 + state + type];
    return state;
}

UNICODE_CODEPOINT utf8_next(const char*& cursor, const char* end) {
    if (cursor == end) return UTF8_END;
    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0;
    const char* p = cursor;
    while (p != end) {
        uint8_t byte = static_cast<uint8_t>(*p++);
        utf8DecodeStep(state, codepoint, byte);
        if (state == UTF8_ACCEPT) {
            cursor = p;
#ifdef UNICODE_BMP_ONLY
            // A valid codepoint that can't be represented in 16 bits (astral
            // planes, or the noncharacters used as sentinels) decodes as the
            // replacement character rather than an error.
            if (codepoint >= 0xFFFE) return UTF8_REPLACEMENT_CHARACTER;
#endif
            return static_cast<UNICODE_CODEPOINT>(codepoint);
        }
        if (state == UTF8_REJECT) return UTF8_ERROR;
    }
    // The buffer ended in the middle of a multi-byte sequence.
    return UTF8_ERROR;
}

size_t utf8_parse(const char* string, UNICODE_CODEPOINT* buf) {
    const char* cursor = string;
    const char* end = string + strlen(string);
    size_t len = 0;
    for (;;) {
        UNICODE_CODEPOINT cp = utf8_next(cursor, end);
        if (cp == UTF8_END) break;
        if (cp == UTF8_ERROR) return 0;
        if (buf != NULL) buf[len] = cp;
        len++;
    }
    return len;
}

size_t utf8_codepoint_length(const char* string) {
    return utf8_parse(string, NULL);
}

}  // namespace focus
