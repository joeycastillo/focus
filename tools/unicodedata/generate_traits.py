#!/usr/bin/env python3
# MIT License
#
# Copyright (c) 2026 Joey Castillo
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
"""
Generate UnicodeTraits.cpp — Unicode character property lookup tables.

Parses UnicodeData.txt, LineBreak.txt, BidiMirroring.txt, and
WordBreakProperty.txt to produce a compact 16-bit-per-codepoint property
table for text layout, bidirectional rendering, and word segmentation.

Bit layout (uint16_t):
  Byte 0 — per-character rendering fields (hot path):
    bits 0–3: bidi_class     (UAX#9 bidirectional class)
    bit  4:   nsm            (nonspacing mark / combining character)
    bit  5:   mirrored       (draw mirrored in RTL context)
    bit  6:   mapped         (a mirror-image mapping exists in BidiMirroring.txt)
    bit  7:   controlchar    (control character)
  Byte 1 — text layout / classification fields:
    bits 8–11: word_break       (UAX#29 Word_Break property)
    bit  12:   whitespace       (whitespace character)
    bit  13:   linebreak        (line break opportunity after this character)
    bits 14–15: (reserved)

Design rationale: bidi_class occupies bits 0–3 so that extracting it in the
rendering hot path is a single mask (packed & 0x000F) with no shift. All other
fields are single-bit checks (packed & CONSTANT), equally efficient at any
position. Byte 0 groups fields needed per-character during glyph rendering;
byte 1 groups fields used during text layout and classification.

Large uniform ranges in UnicodeData.txt (CJK ideographs, Hangul syllables,
surrogates, private use areas) are detected via <..., First>/<..., Last>
sentinel pairs and emitted as hardcoded constants in getTraitsForCodepoint()
rather than as lookup arrays, saving significant binary size.

Usage:
    python3 generate_traits.py [--output PATH]
"""

import argparse
import os
import re
import sys
from collections import OrderedDict

# --- Enum definitions (must match UnicodeTraits.hpp) ---

BIDI_CLASS = {
    'L':    0,
    'R':    1,
    'AL':   2,
    'EN':   3,
    'AN':   4,
    'ES':   5,
    'ET':   6,
    'CS':   7,
    'NSM':  8,
    'ON':   9,
    'WS':   10,
    'BN':   11,
    'B':    12,
    'S':    13,
    # Explicit formatting codes — map to BN (boundary neutral) since we
    # don't implement explicit embedding levels.
    'LRE':  11, 'RLE': 11, 'LRO': 11, 'RLO': 11, 'PDF': 11,
    'LRI':  11, 'RLI': 11, 'FSI': 11, 'PDI': 11,
}

# Combined bidi class for "RNSM" entries that appear in some UnicodeData.txt
# files. Treat as NSM for bidi purposes.
BIDI_CLASS['RNSM'] = BIDI_CLASS['NSM']

WORD_BREAK = {
    'Other':              0,   # Default: punctuation, symbols, separators
    'ALetter':            1,   # Alphabetic letter
    'Hebrew_Letter':      2,   # Hebrew script letter
    'Numeric':            3,   # Digit
    'Katakana':           4,   # Japanese katakana
    'ExtendNumLet':       5,   # Connector (underscore, etc.)
    'Extend':             6,   # Combining mark (merged with Format, ZWJ)
    'MidLetter':          7,   # Mid-word letter separator (U+2019, etc.)
    'MidNum':             8,   # Mid-number separator (comma between digits)
    'MidNumLet':          9,   # Mid-word or mid-number (period, etc.)
    'Single_Quote':       10,  # Apostrophe (U+0027)
    'Double_Quote':       11,  # Quotation mark (U+0022)
    'CR':                 12,  # Carriage return
    'LF':                 13,  # Line feed
    'Newline':            14,  # Other newline characters
    # Merged values — map to an existing category
    'Format':             6,   # → Extend
    'ZWJ':                6,   # → Extend
    'WSegSpace':          0,   # → Other (whitespace bit handles this)
    'Regional_Indicator': 0,   # → Other (emoji flags, not needed)
}

# Line break classes where a break is allowed AFTER the character
LINEBREAK_AFTER = {
    'BA',   # Break After
    'SP',   # Space
    'HY',   # Hyphen
    'SY',   # Symbols allowing breaks (/)
    'ID',   # Ideographic (CJK)
    'EB',   # Emoji Base
    'EM',   # Emoji Modifier
    'AI',   # Ambiguous (treat as ideographic for break purposes)
    'CB',   # Contingent break
}


def pack_traits(bidi_class, nsm, mirrored, mapped, controlchar,
                word_break, whitespace, linebreak):
    """Pack all trait fields into a uint16_t according to the bit layout."""
    return (
        (bidi_class & 0x0F)           |  # bits 0-3
        ((1 if nsm else 0) << 4)      |  # bit 4
        ((1 if mirrored else 0) << 5) |  # bit 5
        ((1 if mapped else 0) << 6)   |  # bit 6
        ((1 if controlchar else 0) << 7) |  # bit 7
        ((word_break & 0x0F) << 8)    |  # bits 8-11
        ((1 if whitespace else 0) << 12) |  # bit 12
        ((1 if linebreak else 0) << 13)  # bit 13
    )


def parse_unicode_data():
    """Parse UnicodeData.txt for character properties.

    Returns:
        traits: dict mapping codepoint -> (bidi_class_str, general_category_str,
                                           bidi_mirrored_bool)
        ranges: list of (start, end, bidi_class_str, general_category_str,
                         bidi_mirrored_bool) for uniform ranges
    """
    traits = {}
    ranges = []
    pending_range_start = None

    with open('UnicodeData.txt', 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            fields = line.split(';')
            if len(fields) < 10:
                continue

            try:
                codepoint = int(fields[0], 16)
            except ValueError:
                continue

            name = fields[1]
            gen_cat = fields[2]
            bidi_cls = fields[4]
            bidi_mirrored = (fields[9] == 'Y')

            # Detect range sentinel pairs
            if ', First>' in name:
                pending_range_start = (codepoint, gen_cat, bidi_cls, bidi_mirrored)
                continue
            elif ', Last>' in name and pending_range_start is not None:
                start_cp, start_gc, start_bidi, start_mirror = pending_range_start
                ranges.append((start_cp, codepoint, start_bidi, start_gc, start_mirror))
                pending_range_start = None
                continue

            traits[codepoint] = (bidi_cls, gen_cat, bidi_mirrored)

    return traits, ranges


def parse_line_breaks():
    """Parse LineBreak.txt for line break properties.

    Returns dict mapping codepoint -> line_break_class string.
    """
    breaks = {}

    with open('LineBreak.txt', 'r') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue

            parts = line.split(';')
            if len(parts) < 2:
                continue

            cp_range = parts[0].strip()
            lb_class = parts[1].strip()

            if '..' in cp_range:
                start, end = cp_range.split('..')
                start = int(start, 16)
                end = int(end, 16)
                for cp in range(start, end + 1):
                    breaks[cp] = lb_class
            else:
                try:
                    cp = int(cp_range, 16)
                    breaks[cp] = lb_class
                except ValueError:
                    continue

    return breaks


def parse_bidi_mirroring():
    """Parse BidiMirroring.txt to find which codepoints have mirror mappings.

    Returns set of codepoints that have a mirror mapping.
    """
    mapped = set()

    with open('BidiMirroring.txt', 'r') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue

            parts = line.split(';')
            if len(parts) < 2:
                continue

            try:
                cp = int(parts[0].strip(), 16)
                mapped.add(cp)
            except ValueError:
                continue

    return mapped


def parse_word_break():
    """Parse WordBreakProperty.txt for word break properties.

    Returns dict mapping codepoint -> word_break_class string.
    """
    breaks = {}

    with open('WordBreakProperty.txt', 'r') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue

            parts = line.split(';')
            if len(parts) < 2:
                continue

            cp_range = parts[0].strip()
            wb_class = parts[1].strip()

            if '..' in cp_range:
                start, end = cp_range.split('..')
                start = int(start, 16)
                end = int(end, 16)
                for cp in range(start, end + 1):
                    breaks[cp] = wb_class
            else:
                try:
                    cp = int(cp_range, 16)
                    breaks[cp] = wb_class
                except ValueError:
                    continue

    return breaks


def compute_packed_value(bidi_cls_str, gen_cat_str, bidi_mirrored, has_mirror_mapping,
                         lb_class, wb_class):
    """Compute the packed uint16_t for a single codepoint's properties."""
    bidi = BIDI_CLASS.get(bidi_cls_str, BIDI_CLASS['ON'])
    wb = WORD_BREAK.get(wb_class, WORD_BREAK['Other'])

    # Derived flags (still use gen_cat_str at generation time)
    nsm = gen_cat_str in ('Mn', 'Me')  # Nonspacing marks and enclosing marks
    controlchar = gen_cat_str == 'Cc'
    whitespace = gen_cat_str in ('Zs', 'Zl', 'Zp') or bidi_cls_str == 'WS'
    linebreak = lb_class in LINEBREAK_AFTER

    return pack_traits(bidi, nsm, bidi_mirrored, has_mirror_mapping, controlchar,
                       wb, whitespace, linebreak)


def build_full_table(traits, ranges, line_breaks, mirror_mapped, word_breaks,
                     max_cp=0xFFFD):
    """Build a complete array of packed values for all codepoints up to max_cp.

    Returns list of (codepoint, packed_value) for all codepoints.
    """
    table = [0] * (max_cp + 1)

    # First, fill in the individual codepoint traits
    for cp in range(max_cp + 1):
        if cp in traits:
            bidi_cls, gen_cat, bidi_mirrored = traits[cp]
        else:
            # Check if this codepoint falls in a uniform range
            in_range = False
            for start, end, bidi_cls_r, gen_cat_r, bidi_mirrored_r in ranges:
                if start <= cp <= end:
                    bidi_cls = bidi_cls_r
                    gen_cat = gen_cat_r
                    bidi_mirrored = bidi_mirrored_r
                    in_range = True
                    break
            if not in_range:
                # Unassigned codepoint
                bidi_cls = 'L'
                gen_cat = 'Cn'
                bidi_mirrored = False

        lb_class = line_breaks.get(cp, 'XX')
        has_mirror = cp in mirror_mapped
        wb_class = word_breaks.get(cp, 'Other')

        table[cp] = compute_packed_value(bidi_cls, gen_cat, bidi_mirrored,
                                         has_mirror, lb_class, wb_class)

    return table


def find_uniform_ranges(table, ranges, max_cp=0xFFFD):
    """Identify uniform ranges where all codepoints share the same packed value.

    Uses the First/Last ranges from UnicodeData.txt. Only considers ranges
    that fall within [0, max_cp] and have at least 128 codepoints.

    Returns list of (start, end, packed_value) sorted by start.
    """
    uniform = []
    for start, end, bidi_cls, gen_cat, bidi_mirrored in ranges:
        # Clamp to our table's range
        if start > max_cp:
            continue
        end = min(end, max_cp)
        if (end - start + 1) < 128:
            continue

        # Verify all values in this range are the same
        val = table[start]
        all_same = all(table[cp] == val for cp in range(start, end + 1))
        if all_same:
            uniform.append((start, end, val))

    uniform.sort(key=lambda x: x[0])
    return uniform


def generate_cpp(table, uniform_ranges, output_path, max_cp=0xFFFD):
    """Generate the C++ source file with lookup arrays and getTraitsForCodepoint()."""

    lines = []
    lines.append('/*')
    lines.append(' * UnicodeTraits.cpp - Unicode character property lookup tables')
    lines.append(' *')
    lines.append(' * AUTO-GENERATED FILE - DO NOT EDIT')
    lines.append(' * Generated by tools/unicodedata/generate_traits.py')
    lines.append(' *')
    lines.append(' * Bit layout (uint16_t):')
    lines.append(' *   bits 0-3:  bidi_class        (in lowest nibble for shift-free extraction)')
    lines.append(' *   bit  4:    nsm               (nonspacing mark)')
    lines.append(' *   bit  5:    mirrored          (draw mirrored in RTL)')
    lines.append(' *   bit  6:    mapped            (mirror mapping exists)')
    lines.append(' *   bit  7:    controlchar       (control character)')
    lines.append(' *   bits 8-11: word_break         (UAX#29 Word_Break property)')
    lines.append(' *   bit  12:   whitespace')
    lines.append(' *   bit  13:   linebreak         (line break opportunity after)')
    lines.append(' *   bits 14-15: (reserved)')
    lines.append(' */')
    lines.append('')
    lines.append('#include "UnicodeTraits.hpp"')
    lines.append('')

    # Build the list of array segments (gaps between uniform ranges)
    segments = []  # (start, end, array_name)
    prev_end = 0

    for u_start, u_end, u_val in uniform_ranges:
        if prev_end < u_start:
            # There's a gap that needs an array
            segments.append((prev_end, u_start - 1))
        prev_end = u_end + 1

    # Final segment after last uniform range
    if prev_end <= max_cp:
        segments.append((prev_end, max_cp))

    # Emit arrays for each segment
    for seg_start, seg_end in segments:
        array_name = f'_unicode_info_{seg_start:04X}_{seg_end:04X}'
        count = seg_end - seg_start + 1

        lines.append(f'extern const uint16_t {array_name}[] = {{')

        # Emit values 16 per line
        for i in range(0, count, 16):
            chunk = []
            for j in range(i, min(i + 16, count)):
                cp = seg_start + j
                chunk.append(f'0x{table[cp]:04X}')
            prefix = '    '
            lines.append(prefix + ', '.join(chunk) + ',')

        lines.append('};')
        lines.append('')

    # Emit getTraitsForCodepoint()
    lines.append('unicode_info_t getTraitsForCodepoint(UNICODE_CODEPOINT codepoint) {')
    lines.append('    unicode_info_t retval;')
    lines.append('')

    # Build the if/else chain
    first = True
    prev_end = 0

    for u_start, u_end, u_val in uniform_ranges:
        # Array segment before this uniform range
        if prev_end < u_start:
            seg_start = prev_end
            seg_end = u_start - 1
            array_name = f'_unicode_info_{seg_start:04X}_{seg_end:04X}'
            keyword = 'if' if first else 'else if'
            if seg_start == 0:
                lines.append(f'    {keyword} (codepoint < 0x{u_start:04X}) retval.packed = {array_name}[codepoint];')
            else:
                lines.append(f'    {keyword} (codepoint < 0x{u_start:04X}) retval.packed = {array_name}[codepoint - 0x{seg_start:04X}];')
            first = False

        # Uniform range
        keyword = 'if' if first else 'else if'
        lines.append(f'    {keyword} (codepoint <= 0x{u_end:04X}) retval.packed = 0x{u_val:04X}; // {u_start:04X}-{u_end:04X} uniform range')
        first = False
        prev_end = u_end + 1

    # Final array segment
    if prev_end <= max_cp:
        seg_start = prev_end
        seg_end = max_cp
        array_name = f'_unicode_info_{seg_start:04X}_{seg_end:04X}'
        keyword = 'if' if first else 'else if'
        lines.append(f'    {keyword} (codepoint <= 0x{max_cp:04X}) retval.packed = {array_name}[codepoint - 0x{seg_start:04X}];')

    lines.append('    else retval.packed = 0xFFFF; // not a character')
    lines.append('')
    lines.append('    return retval;')
    lines.append('}')
    lines.append('')

    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))

    # Stats
    total_array_entries = sum(seg_end - seg_start + 1 for seg_start, seg_end in segments)
    total_uniform = sum(u_end - u_start + 1 for u_start, u_end, _ in uniform_ranges)
    array_bytes = total_array_entries * 2  # uint16_t

    print(f"Generated {output_path}")
    print(f"  Codepoints covered: 0x0000-0x{max_cp:04X} ({max_cp + 1} total)")
    print(f"  Array entries: {total_array_entries} ({array_bytes} bytes)")
    print(f"  Uniform range entries: {total_uniform} (saved {total_uniform * 2} bytes)")
    print(f"  Uniform ranges: {len(uniform_ranges)}")
    for u_start, u_end, u_val in uniform_ranges:
        print(f"    0x{u_start:04X}-0x{u_end:04X}: 0x{u_val:04X} ({u_end - u_start + 1} codepoints)")


def main():
    parser = argparse.ArgumentParser(description='Generate UnicodeTraits.cpp')
    parser.add_argument('--output', '-o',
                        default='../../components/focus/src/text/UnicodeTraits.cpp',
                        help='Output file path')
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    print("Parsing UnicodeData.txt...")
    traits, ranges = parse_unicode_data()
    print(f"  {len(traits)} individual codepoints, {len(ranges)} ranges")

    print("Parsing LineBreak.txt...")
    line_breaks = parse_line_breaks()
    print(f"  {len(line_breaks)} line break entries")

    print("Parsing BidiMirroring.txt...")
    mirror_mapped = parse_bidi_mirroring()
    print(f"  {len(mirror_mapped)} mirror mappings")

    print("Parsing WordBreakProperty.txt...")
    word_breaks = parse_word_break()
    print(f"  {len(word_breaks)} word break entries")

    print("Building full table...")
    table = build_full_table(traits, ranges, line_breaks, mirror_mapped, word_breaks)

    print("Finding uniform ranges...")
    uniform_ranges = find_uniform_ranges(table, ranges)

    generate_cpp(table, uniform_ranges, args.output)

    # Sanity checks
    print("\nSanity checks:")
    # Reverse lookup for word_break names (exclude merged aliases)
    wb_names = {}
    for k, v in WORD_BREAK.items():
        if k not in ('Format', 'ZWJ', 'WSegSpace', 'Regional_Indicator'):
            wb_names.setdefault(v, k)

    checks = [
        (0x0041, 'A',          'ALetter'),
        (0x0028, '(',          'Other, mirrored+mapped'),
        (0x0030, '0',          'Numeric'),
        (0x0020, 'space',      'Other, whitespace+linebreak'),
        (0x05D0, 'aleph',      'Hebrew_Letter'),
        (0x0627, 'alef',       'ALetter (Arabic)'),
        (0x002D, '-',          'Other, linebreak'),
        (0x002C, ',',          'MidNum'),
        (0x002E, '.',          'MidNumLet'),
        (0x0027, "'",          'Single_Quote'),
        (0x2019, '\u2019',     'MidNumLet'),
        (0x005F, '_',          'ExtendNumLet'),
    ]
    for cp, name, desc in checks:
        val = table[cp]
        bidi = val & 0x0F
        nsm = bool(val & 0x10)
        mirrored = bool(val & 0x20)
        mapped = bool(val & 0x40)
        ctrl = bool(val & 0x80)
        wb = (val >> 8) & 0x0F
        ws = bool(val & 0x1000)
        lb = bool(val & 0x2000)
        bidi_rev = {v: k for k, v in BIDI_CLASS.items() if v < 14}
        print(f"  U+{cp:04X} ({name}): 0x{val:04X} "
              f"bidi={bidi_rev.get(bidi, '?')} wb={wb_names.get(wb, '?')} "
              f"{'nsm ' if nsm else ''}{'mirror ' if mirrored else ''}"
              f"{'mapped ' if mapped else ''}{'ctrl ' if ctrl else ''}"
              f"{'ws ' if ws else ''}{'lb ' if lb else ''}"
              f"[{desc}]")


if __name__ == '__main__':
    main()
