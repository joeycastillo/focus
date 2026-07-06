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
Convert a BDF font to BDP (Bitmap Distribution Packed), the format that
Focus's PackedFontGlyphProvider loads. BDP is a binary encoding of BDF glyph
data that eliminates text parsing at load time.

Usage:
    python3 bdf_to_bdp.py input.bdf output.bdp [--title "Display Name"]

See README.md for the BDP format specification.
"""

import argparse
import struct
import sys


def parse_bdf(path):
    """Parse a BDF font file into (font_info, glyphs).

    glyphs maps codepoint -> {advance, width, height, xOffset, yOffset, bitmap},
    where bitmap is tightly packed: ((width+7)//8) * height bytes, MSB-first.
    """
    font_info = {
        'pixelSize': 0, 'fontAscent': 0, 'fontDescent': 0,
        'maxWidth': 0, 'maxHeight': 0, 'defaultChar': 0,
    }
    glyphs = {}

    in_char = False
    in_bitmap = False
    current_encoding = None
    current_glyph = None
    raw_bitmap = []

    with open(path, 'r') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if not line:
                continue

            parts = line.split(None, 1)
            if not parts:
                continue
            keyword = parts[0]
            rest = parts[1] if len(parts) > 1 else ''

            if not in_char:
                if keyword == 'PIXEL_SIZE':
                    font_info['pixelSize'] = int(rest)
                elif keyword == 'FONT_ASCENT':
                    font_info['fontAscent'] = int(rest)
                elif keyword == 'FONT_DESCENT':
                    font_info['fontDescent'] = int(rest)
                elif keyword == 'FONTBOUNDINGBOX':
                    vals = rest.split()
                    font_info['maxWidth'] = int(vals[0])
                    font_info['maxHeight'] = int(vals[1])
                elif keyword == 'DEFAULT_CHAR':
                    font_info['defaultChar'] = int(rest)
                elif keyword == 'STARTCHAR':
                    in_char = True
                    current_encoding = None
                    current_glyph = {'width': 0, 'height': 0,
                                     'xOffset': 0, 'yOffset': 0, 'advance': 0}
                    raw_bitmap = []
            else:
                if keyword == 'ENCODING':
                    # BDF may add a second field (the intended codepoint) when
                    # the primary encoding is -1; take the first.
                    current_encoding = int(rest.split()[0])
                    if current_encoding < 0:
                        # No standard encoding; skip the glyph.
                        in_char = False
                        in_bitmap = False
                elif keyword == 'DWIDTH':
                    current_glyph['advance'] = int(rest.split()[0])
                elif keyword == 'BBX':
                    vals = rest.split()
                    current_glyph['width'] = int(vals[0])
                    current_glyph['height'] = int(vals[1])
                    current_glyph['xOffset'] = int(vals[2])
                    current_glyph['yOffset'] = int(vals[3])
                elif keyword == 'BITMAP':
                    in_bitmap = True
                elif keyword == 'ENDCHAR':
                    bytes_per_row = (current_glyph['width'] + 7) // 8
                    bitmap = bytearray()
                    for hex_line in raw_bitmap:
                        row = bytes(int(hex_line[i:i+2], 16)
                                    for i in range(0, len(hex_line) - 1, 2))
                        # BDF pads rows to a byte boundary; keep the tight bytes.
                        bitmap.extend(row[:bytes_per_row])
                    current_glyph['bitmap'] = bytes(bitmap)
                    if current_encoding is not None:
                        glyphs[current_encoding] = current_glyph
                    in_char = False
                    in_bitmap = False
                elif in_bitmap:
                    raw_bitmap.append(line.strip())

    return font_info, glyphs


def write_bdp(path, font_info, glyphs, title=''):
    """Write a BDP file from parsed BDF data."""
    title_bytes = title.encode('utf-8')[:255]
    sorted_codepoints = sorted(glyphs.keys())

    # BDP packs the count and per-glyph metrics in fixed-width fields; report
    # the offending value with context instead of letting struct.pack raise bare.
    if len(sorted_codepoints) > 0xFFFF:
        raise ValueError(f"{len(sorted_codepoints)} glyphs exceeds BDP's maximum of 65535")
    if not 0 <= font_info['defaultChar'] <= 0xFFFFFFFF:
        raise ValueError(f"DEFAULT_CHAR={font_info['defaultChar']} does not fit BDP's uint32")
    for cp in sorted_codepoints:
        g = glyphs[cp]
        for field, val, lo, hi in (('codepoint', cp, 0, 0xFFFFFFFF),
                                   ('advance', g['advance'], 0, 255),
                                   ('width', g['width'], 0, 255),
                                   ('height', g['height'], 0, 255),
                                   ('xOffset', g['xOffset'], -128, 127),
                                   ('yOffset', g['yOffset'], -128, 127)):
            if not lo <= val <= hi:
                raise ValueError(f"U+{cp:04X}: {field}={val} does not fit BDP's [{lo}, {hi}]")

    with open(path, 'wb') as f:
        f.write(b'BDP\x01')
        f.write(struct.pack('<BBBBB',
            font_info['pixelSize'], font_info['fontAscent'],
            font_info['fontDescent'], font_info['maxWidth'],
            font_info['maxHeight']))
        f.write(struct.pack('<B', len(title_bytes)))
        f.write(struct.pack('<H', len(sorted_codepoints)))
        f.write(struct.pack('<I', font_info['defaultChar']))

        if title_bytes:
            f.write(title_bytes)

        for cp in sorted_codepoints:
            g = glyphs[cp]
            f.write(struct.pack('<IBBBbb', cp, g['advance'], g['width'],
                                g['height'], g['xOffset'], g['yOffset']))

        for cp in sorted_codepoints:
            f.write(glyphs[cp]['bitmap'])


def main():
    ap = argparse.ArgumentParser(
        description="Convert a BDF font to BDP (the format PackedFontGlyphProvider loads).")
    ap.add_argument('input', help='input .bdf file')
    ap.add_argument('output', help='output .bdp file')
    ap.add_argument('--title', default='',
                    help='display title packed into the file (UTF-8, <=255 bytes)')
    args = ap.parse_args()

    try:
        font_info, glyphs = parse_bdf(args.input)
        if not glyphs:
            print(f"No glyphs parsed from {args.input}", file=sys.stderr)
            return 1
        write_bdp(args.output, font_info, glyphs, args.title)
    except (OSError, ValueError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    print(f"{args.input} -> {args.output}: {len(glyphs)} glyphs")
    return 0


if __name__ == '__main__':
    sys.exit(main())
