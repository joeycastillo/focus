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
 * @file UnicodeArabicPresentation.hpp
 * @brief Arabic letter shaping (presentation forms) lookup.
 *
 * Arabic letters take different visual forms depending on their position
 * in a word. This module provides lookup tables to convert base Arabic
 * characters (U+0621-U+06D2) to their presentation forms.
 *
 * Note: Lam-Alef ligatures must be handled separately by the shaping engine.
 * The ligature mappings are:
 *   U+0644 + U+0622 => U+FEF5 (isolated) / U+FEF6 (final)
 *   U+0644 + U+0623 => U+FEF7 (isolated) / U+FEF8 (final)
 *   U+0644 + U+0625 => U+FEF9 (isolated) / U+FEFA (final)
 *   U+0644 + U+0627 => U+FEFB (isolated) / U+FEFC (final)
 */

#pragma once

#include <cstdint>
#include "Utf8.hpp"

namespace focus {

namespace UnicodeArabicPresentation {

/**
 * @brief Positional forms for Arabic letters.
 */
enum class Form {
    Isolated = 0,  ///< Standalone letter (not connected)
    Initial = 1,   ///< Beginning of word (connected on left)
    Medial = 2,    ///< Middle of word (connected on both sides)
    Final = 3      ///< End of word (connected on right)
};

/**
 * @brief Check if a codepoint participates in Arabic shaping.
 * @param cp The codepoint to check (should be in U+0621-U+06D2 range).
 * @return true if the codepoint has presentation forms available.
 */
bool isShapeable(UNICODE_CODEPOINT cp);

/**
 * @brief Get the presentation form for an Arabic letter.
 * @param base The base Arabic codepoint (e.g., U+0628 for Beh).
 * @param form The desired positional form.
 * @return The presentation form codepoint, or 0 if the form doesn't exist.
 *
 * Example:
 *   getForm(0x0628, Form::Initial) returns 0xFE91 (Beh initial form)
 */
UNICODE_CODEPOINT getForm(UNICODE_CODEPOINT base, Form form);

} // namespace UnicodeArabicPresentation

}  // namespace focus
