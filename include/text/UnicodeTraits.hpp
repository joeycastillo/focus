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
 *     bits 8–11: word_break       (UAX#29 Word_Break property)
 *     bit  12:   whitespace       (whitespace character)
 *     bit  13:   linebreak        (line break opportunity after this character)
 *     bits 14–15: (reserved)
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
 * @brief UAX#29 Word_Break property values (4 bits, 0–14).
 *
 * Some Unicode Word_Break values are merged for compactness:
 *   Format, ZWJ → WB_Extend
 *   WSegSpace, Regional_Indicator → WB_Other
 */
enum WordBreak : uint8_t {
    WB_Other         = 0,   ///< Default: punctuation, symbols, separators — always a word boundary
    WB_ALetter       = 1,   ///< Alphabetic letter
    WB_Hebrew_Letter = 2,   ///< Hebrew script letter
    WB_Numeric       = 3,   ///< Digit
    WB_Katakana      = 4,   ///< Japanese katakana
    WB_ExtendNumLet  = 5,   ///< Connector (underscore, etc.)
    WB_Extend        = 6,   ///< Combining mark, format character, or ZWJ
    WB_MidLetter     = 7,   ///< Mid-word letter separator (U+2019, U+00B7, etc.)
    WB_MidNum        = 8,   ///< Mid-number separator (comma between digits, etc.)
    WB_MidNumLet     = 9,   ///< Mid-word or mid-number (period, etc.)
    WB_Single_Quote  = 10,  ///< Apostrophe (U+0027)
    WB_Double_Quote  = 11,  ///< Quotation mark (U+0022)
    WB_CR            = 12,  ///< Carriage return
    WB_LF            = 13,  ///< Line feed
    WB_Newline       = 14,  ///< Other newline characters
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
        uint16_t word_break: 4;       ///< UAX#29 Word_Break property (WordBreak enum)
        uint16_t whitespace: 1;       ///< Is a whitespace character
        uint16_t linebreak: 1;        ///< A line break opportunity exists after this character
        uint16_t _reserved: 2;
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
