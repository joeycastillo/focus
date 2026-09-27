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
 * @file InlineObjectProvider.hpp
 * @brief Interface for inline objects laid out within a line of text.
 *
 * An inline object is a U+FFFC OBJECT REPLACEMENT CHARACTER in the code
 * point stream. An InlineObjectProvider gives each one a width and learns
 * where it lands; the caller draws the object itself. Inline objects are
 * for code that does its own line layout, with TextLayout::measureLineWrap
 * and a CanvasView subclass's renderBidiLine calls. Built-in views and
 * CanvasView::drawText do not lay them out.
 *
 * @ingroup text
 */

#pragma once

#include "Utf8.hpp"
#include <cstdint>

namespace focus {

class InlineObjectProvider {
public:
    virtual ~InlineObjectProvider() = default;

    /// Width of an inline object in pixels, at the current text size.
    /// @param object Points at the U+FFFC in the buffer passed to the
    ///        layout call. Valid only during this call; subtract the
    ///        buffer's base to get the object's index.
    virtual int16_t widthOfObject(const UNICODE_CODEPOINT* object) const = 0;

    /// Called once per object as renderBidiLine places it.
    /// @param object Points at the U+FFFC, as for widthOfObject.
    /// @param x The object's left edge in canvas coordinates.
    virtual void objectPlaced(const UNICODE_CODEPOINT* object, int16_t x) {}
};

}  // namespace focus
