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

#include "ArabicShaping.hpp"
#include "UnicodeArabicPresentation.hpp"
#include "UnicodeTraits.hpp"

using namespace UnicodeArabicPresentation;

static inline bool isInArabicBlock(UNICODE_CODEPOINT cp) {
    return (cp >> 8) == 0x06;
}

void shapeArabic(UNICODE_CODEPOINT* codepoints, size_t len) {
    // Track whether the previous shaped character connects forward.
    // This avoids looking backward at already-shaped codepoints whose
    // values have been replaced with presentation forms.
    bool prevConnectsForward = false;

    for (size_t i = 0; i < len; i++) {
        UNICODE_CODEPOINT cp = codepoints[i];

        // Non-Arabic characters break connectivity
        if (!isInArabicBlock(cp)) {
            prevConnectsForward = false;
            continue;
        }

        // Non-spacing marks (diacritics like fathah, kasrah, etc.)
        // don't affect shaping connectivity — skip them
        if (getTraitsForCodepoint(cp).is.nsm) continue;

        // Characters without presentation forms (numerals, punctuation)
        // also break connectivity
        if (!isShapeable(cp)) {
            prevConnectsForward = false;
            continue;
        }

        // Look forward past non-spacing marks to find the next
        // connectable character
        UNICODE_CODEPOINT next = 0;
        size_t nextIndex = 0;
        for (size_t j = i + 1; j < len; j++) {
            UNICODE_CODEPOINT candidate = codepoints[j];
            // Skip Arabic NSMs
            if (isInArabicBlock(candidate) && getTraitsForCodepoint(candidate).is.nsm) {
                continue;
            }
            // Found the next non-NSM character
            if (isShapeable(candidate)) {
                // It can connect backward if it has medial or final forms
                if (getForm(candidate, Form::Medial) != 0 ||
                    getForm(candidate, Form::Final) != 0) {
                    next = candidate;
                    nextIndex = j;
                }
            }
            break;
        }

        bool hasPrev = prevConnectsForward;
        bool hasNext = (next != 0);

        // Handle Lam-Alef ligatures
        if (cp == 0x0644 && hasNext &&
            (next == 0x0622 || next == 0x0623 ||
             next == 0x0625 || next == 0x0627)) {
            UNICODE_CODEPOINT ligature = 0;
            switch (next) {
                case 0x0622: ligature = hasPrev ? 0xFEF6 : 0xFEF5; break;
                case 0x0623: ligature = hasPrev ? 0xFEF8 : 0xFEF7; break;
                case 0x0625: ligature = hasPrev ? 0xFEFA : 0xFEF9; break;
                case 0x0627: ligature = hasPrev ? 0xFEFC : 0xFEFB; break;
            }
            codepoints[i] = ligature;
            codepoints[nextIndex] = 0x200B; // Zero Width Space: Alef consumed by ligature
            prevConnectsForward = false; // Ligature is a terminal form
            continue;
        }

        // Choose the contextual form based on neighbors
        if (hasPrev && hasNext && getForm(cp, Form::Medial) != 0) {
            codepoints[i] = getForm(cp, Form::Medial);
            prevConnectsForward = true;
        } else if (hasNext && getForm(cp, Form::Initial) != 0) {
            codepoints[i] = getForm(cp, Form::Initial);
            prevConnectsForward = true;
        } else if (hasPrev && getForm(cp, Form::Final) != 0) {
            codepoints[i] = getForm(cp, Form::Final);
            prevConnectsForward = false;
        } else {
            UNICODE_CODEPOINT isolated = getForm(cp, Form::Isolated);
            if (isolated != 0) {
                codepoints[i] = isolated;
            }
            prevConnectsForward = false;
        }
    }
}
