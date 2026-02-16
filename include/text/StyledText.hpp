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
 * @file StyledText.hpp
 * @brief Generic attributed text types for the Focus text layout engine.
 *
 * StyleRun attaches layout instructions (page breaks, indentation changes,
 * extra spacing, title formatting) to byte positions in a UTF-8 text stream.
 * The text itself is not modified — control codes remain in-band and style
 * runs annotate positions where the layout engine should adjust behavior.
 *
 * This is a data-only header with no dependencies beyond the standard library.
 */

#pragma once

#include <cstdint>
#include <vector>

/// Layout instruction types that can be attached to byte positions.
enum class TextStyle : uint8_t {
    PageBreakBefore,   ///< Force a page break before this byte offset.
    SceneBreak,        ///< Extra vertical whitespace at this byte offset.
    IndentLevel,       ///< Block quote nesting depth (value = level, 0 = no indent).
    TitleMode,         ///< Next line is a title: render bold, extra spacing after.
};

/// A layout instruction at a specific byte position in the text stream.
struct StyleRun {
    uint32_t byteOffset;   ///< Byte offset in the text where this style takes effect.
    TextStyle style;       ///< The layout instruction type.
    uint8_t value;         ///< Meaning depends on style (e.g. indent level, or 1 for boolean styles).
    uint16_t consumeBytes = 0; ///< Bytes to skip past at this position (e.g. DLE+'>' prefix bytes).
};
