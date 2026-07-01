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
 * @file FontStyle.hpp
 * @brief The text emphasis type requested from a glyph provider.
 */

#pragma once

#include <stdint.h>

namespace focus {

/**
 * @brief Text emphasis as a set of style flags.
 *
 * Passed to GlyphProvider and Font when requesting a glyph or its metrics, so a
 * font family can hand back the matching styled variant. The flags combine with
 * `|`: `FontStyle::Italic | FontStyle::Bold` requests bold italic. Regular is the
 * absence of both. A provider that lacks a variant may synthesize it (a shear for
 * italic, a doublestrike for bold) or fall back to Regular.
 * @ingroup text
 */
enum class FontStyle : uint8_t {
    Regular = 0,       ///< Upright, normal weight.
    Italic  = 1 << 0,  ///< Slanted.
    Bold    = 1 << 1,  ///< Heavier weight.
};

/**
 * @brief Union of two styles, e.g. `FontStyle::Italic | FontStyle::Bold`.
 */
constexpr FontStyle operator|(FontStyle a, FontStyle b) {
    return static_cast<FontStyle>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/**
 * @brief Intersection of two styles; empty (Regular) when they share no flag.
 */
constexpr FontStyle operator&(FontStyle a, FontStyle b) {
    return static_cast<FontStyle>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/**
 * @brief Complement within the defined flags, e.g. `~FontStyle::Bold == FontStyle::Italic`.
 */
constexpr FontStyle operator~(FontStyle a) {
    constexpr uint8_t all = static_cast<uint8_t>(FontStyle::Italic) | static_cast<uint8_t>(FontStyle::Bold);
    return static_cast<FontStyle>(~static_cast<uint8_t>(a) & all);
}

constexpr FontStyle& operator|=(FontStyle& a, FontStyle b) { return a = a | b; }
constexpr FontStyle& operator&=(FontStyle& a, FontStyle b) { return a = a & b; }

/**
 * @brief Whether `style` includes `flag`, e.g. `contains(style, FontStyle::Bold)`.
 */
constexpr bool contains(FontStyle style, FontStyle flag) {
    return (static_cast<uint8_t>(style) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag);
}

}  // namespace focus
