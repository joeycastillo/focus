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
 * @file Hyphenator.hpp
 * @brief Abstract interface for word hyphenation.
 *
 * Hyphenator defines the contract for hyphenation backends that can identify
 * legal break positions within a word. Implementations provide language-specific
 * hyphenation rules (e.g. Liang algorithm with TeX patterns). The text layout
 * engine uses this interface to improve line breaking by splitting long words
 * at legal hyphenation points.
 *
 * @ingroup text
 */

#pragma once

#include "Focus.hpp"
#include "utf8_decode.hpp"
#include <cstddef>

namespace focus {

class Hyphenator {
public:
    virtual ~Hyphenator() = default;

    /// Find legal hyphenation break positions within a word.
    /// @param word Array of Unicode codepoints representing the word.
    /// @param wordLen Number of codepoints in the word.
    /// @param positions Output array for break positions (codepoint indices).
    ///        Each position indicates that a break is legal AFTER that
    ///        codepoint index (i.e., the prefix is word[0..positions[i]]).
    /// @param maxPositions Size of the output array.
    /// @return Number of positions written (0 if word cannot be hyphenated).
    virtual size_t findBreakPositions(
        const UNICODE_CODEPOINT* word, size_t wordLen,
        size_t* positions, size_t maxPositions
    ) const = 0;
};

}  // namespace focus
