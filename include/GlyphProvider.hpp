/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
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

#include "Focus.hpp"
#include "utf8_decode.hpp"

class GlyphProvider : public std::enable_shared_from_this<GlyphProvider> {
public:
    GlyphProvider();
    virtual uint8_t getPointSize() = 0;
    virtual Size getMaxSize() = 0;
    virtual Point getOffset() = 0;
    virtual uint8_t getGlyphRowCount() = 0;
    virtual bool isValid() const = 0;

    virtual uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) = 0;
    virtual Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) = 0;
};
