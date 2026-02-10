/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Joey Castillo
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
 * @file UnicodeTraits.hpp
 * @brief Unicode character property lookup for text layout and bidirectional support.
 *
 * Provides a compact 16-bit bitfield of Unicode properties for any codepoint,
 * including bidi class, general category, whitespace classification, line break
 * opportunities, combining mark detection, and mirroring flags. Used by the
 * text layout engine for word wrapping and by renderers for bidirectional text.
 *
 * Bit layout (uint16_t):
 *   Byte 0 — per-character rendering fields (hot path):
 *     bits 0–3: bidi_class     (UAX#9 bidirectional class, in lowest nibble
 *                                for shift-free extraction: packed & 0x000F)
 *     bit  4:   nsm            (nonspacing mark / combining character)
 *     bit  5:   mirrored       (draw mirrored in RTL context)
 *     bit  6:   mapped         (a mirror-image mapping exists)
 *     bit  7:   controlchar    (control character)
 *   Byte 1 — text layout / classification fields:
 *     bits 8–12: general_category (Unicode General Category)
 *     bit  13:   whitespace       (whitespace character)
 *     bit  14:   linebreak        (line break opportunity after this character)
 *     bit  15:   (reserved)
 */

#pragma once

#include <stdint.h>
#include "utf8_decode.hpp"

/**
 * @brief UAX#9 Bidirectional class values.
 *
 * Stored in the lowest 4 bits of unicode_info_t for efficient extraction.
 */
enum BidiClass : uint8_t {
    BIDI_L   = 0,   ///< Strong left-to-right (Latin, CJK, etc.)
    BIDI_R   = 1,   ///< Strong right-to-left (Hebrew)
    BIDI_AL  = 2,   ///< Strong right-to-left Arabic
    BIDI_EN  = 3,   ///< European number (digits 0-9)
    BIDI_AN  = 4,   ///< Arabic number
    BIDI_ES  = 5,   ///< European number separator (+, -)
    BIDI_ET  = 6,   ///< European number terminator ($, %, etc.)
    BIDI_CS  = 7,   ///< Common number separator (. , :)
    BIDI_NSM = 8,   ///< Nonspacing mark (combining diacritical, etc.)
    BIDI_ON  = 9,   ///< Other neutral (parentheses, symbols)
    BIDI_WS  = 10,  ///< Whitespace
    BIDI_BN  = 11,  ///< Boundary neutral (control codes, formatting)
    BIDI_B   = 12,  ///< Paragraph separator
    BIDI_S   = 13,  ///< Segment separator
};

/**
 * @brief Unicode General Category values (5 bits, 0–31).
 */
enum GeneralCategory : uint8_t {
    GC_Lu = 0,   ///< Letter, uppercase
    GC_Ll = 1,   ///< Letter, lowercase
    GC_Lt = 2,   ///< Letter, titlecase
    GC_Lm = 3,   ///< Letter, modifier
    GC_Lo = 4,   ///< Letter, other
    GC_Mn = 5,   ///< Mark, nonspacing
    GC_Mc = 6,   ///< Mark, spacing combining
    GC_Me = 7,   ///< Mark, enclosing
    GC_Nd = 8,   ///< Number, decimal digit
    GC_Nl = 9,   ///< Number, letter
    GC_No = 10,  ///< Number, other
    GC_Pc = 11,  ///< Punctuation, connector
    GC_Pd = 12,  ///< Punctuation, dash
    GC_Ps = 13,  ///< Punctuation, open
    GC_Pe = 14,  ///< Punctuation, close
    GC_Pi = 15,  ///< Punctuation, initial quote
    GC_Pf = 16,  ///< Punctuation, final quote
    GC_Po = 17,  ///< Punctuation, other
    GC_Sm = 18,  ///< Symbol, math
    GC_Sc = 19,  ///< Symbol, currency
    GC_Sk = 20,  ///< Symbol, modifier
    GC_So = 21,  ///< Symbol, other
    GC_Zs = 22,  ///< Separator, space
    GC_Zl = 23,  ///< Separator, line
    GC_Zp = 24,  ///< Separator, paragraph
    GC_Cc = 25,  ///< Other, control
    GC_Cf = 26,  ///< Other, format
    GC_Cs = 27,  ///< Other, surrogate
    GC_Co = 28,  ///< Other, private use
    GC_Cn = 29,  ///< Other, not assigned
};

/**
 * @brief Packed bitfield of Unicode character properties.
 *
 * Can be accessed either as individual fields via the `is` struct, or
 * as a single packed uint16_t for efficient comparison and storage.
 */
typedef union {
    struct {
        // Byte 0 — per-character rendering fields (hot path)
        uint16_t bidi_class: 4;        ///< UAX#9 bidirectional class (BidiClass enum)
        uint16_t nsm: 1;              ///< Is a nonspacing mark (combining diacritical, etc.)
        uint16_t mirrored: 1;         ///< Should be drawn mirrored in RTL text runs
        uint16_t mapped: 1;           ///< A mirror-image mapping exists for this character
        uint16_t controlchar: 1;      ///< Is a control character (U+0000..U+001F, etc.)
        // Byte 1 — text layout / classification fields
        uint16_t general_category: 5; ///< Unicode General Category (GeneralCategory enum)
        uint16_t whitespace: 1;       ///< Is a whitespace character
        uint16_t linebreak: 1;        ///< A line break opportunity exists after this character
        uint16_t _reserved: 1;
    } is;
    uint16_t packed; ///< All fields packed into a single 16-bit value.
} unicode_info_t;

/**
 * @brief Helper to test if a bidi class represents strong RTL directionality.
 */
static inline bool bidiIsRTL(uint8_t bidi_class) {
    return bidi_class == BIDI_R || bidi_class == BIDI_AL;
}

/**
 * @brief Helper to test if a bidi class represents strong LTR directionality.
 */
static inline bool bidiIsLTR(uint8_t bidi_class) {
    return bidi_class == BIDI_L;
}

/**
 * @brief Helper to test if a bidi class is strong (L, R, or AL).
 */
static inline bool bidiIsStrong(uint8_t bidi_class) {
    return bidi_class <= BIDI_AL;  // L=0, R=1, AL=2
}

/**
 * @brief Look up Unicode properties for a codepoint.
 * @param codepoint The Unicode codepoint to query.
 * @return A unicode_info_t with the relevant property fields set.
 */
unicode_info_t getTraitsForCodepoint(UNICODE_CODEPOINT codepoint);
