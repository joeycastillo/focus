#!/usr/bin/env python3
# Copyright (c) 2026 Joey Castillo
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, see <https://www.gnu.org/licenses/>.
"""
Generate unifont.bin - a simplified glyph-only font file for Unifont.

This creates a compact binary format optimized for O(1) glyph lookup by codepoint.

Usage:
    python3 generate_unifont.py [--output PATH] [--last-codepoint HEX]

Output format:
    Header (16 bytes):
        0-3:   Magic "UFNT"
        4-5:   Version (0x0001)
        6:     Nominal width (8)
        7:     Nominal height (16)
        8-11:  Total glyph count
        12-15: Reserved

    Per-plane descriptor (8 bytes each, 3 planes):
        0-1:   First codepoint (low 16 bits)
        2-3:   Last codepoint (low 16 bits)
        4-7:   Offset to plane's lookup table

    Lookup table (4 bytes per codepoint, dense):
        Bits 0-23:  Offset to glyph data (0 if no glyph)
        Bits 24-28: Width (0-31, typically 8 or 16)
        Bits 29-31: Reserved

    Glyph data:
        16 bytes for 8-wide glyphs
        32 bytes for 16-wide glyphs
"""

import struct
import argparse
import os

# Default: include all of Planes 0-2 (full Unifont coverage)
DEFAULT_LAST_CODEPOINT = 0x2FFFF

def is_debug_glyph(glyph_bytes, width):
    """Check if a glyph is a Unifont debugging/informational placeholder.

    Unifont includes visible labeled boxes for invisible characters (control codes,
    format characters, variation selectors, etc.). These are useful for font debugging
    but should not appear in normal text rendering.

    Three patterns exist for 16-wide debug glyphs:
    - Dashed border: starts with AAAA 0001
    - Inverted dashed border: starts with 5555, ends with AAAA
    - Solid border: starts with 0000 FFFF, ends with FFFF 0000
    """
    if width == 16 and len(glyph_bytes) == 32:
        # Dashed border: starts with AAAA 0001
        if glyph_bytes[0:4] == b'\xAA\xAA\x00\x01':
            return True
        # Inverted dashed border: starts with 5555, ends with AAAA
        if glyph_bytes[0:2] == b'\x55\x55' and glyph_bytes[30:32] == b'\xAA\xAA':
            return True
        # Solid border: starts with 0000 FFFF, ends with FFFF 0000
        if glyph_bytes[0:4] == b'\x00\x00\xFF\xFF' and glyph_bytes[28:32] == b'\xFF\xFF\x00\x00':
            return True
    return False

def parse_hex_file(filename, glyphs):
    """Parse a Unifont .hex file into the glyphs dict."""
    if not os.path.exists(filename):
        print(f"Warning: {filename} not found, skipping")
        return

    debug_stripped = 0
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or ':' not in line:
                continue
            parts = line.split(':')
            if len(parts) < 2:
                continue

            codepoint_str = parts[0].lstrip('0') or '0'
            try:
                codepoint = int(codepoint_str, 16)
            except ValueError:
                continue

            hex_data = parts[1]
            glyph_bytes = bytes.fromhex(hex_data)

            # Determine width from data length
            if len(glyph_bytes) == 16:
                width = 8
            elif len(glyph_bytes) == 32:
                width = 16
            else:
                # Non-standard size, skip
                continue

            # Strip Unifont debugging glyphs (labeled boxes for invisible characters)
            if is_debug_glyph(glyph_bytes, width):
                debug_stripped += 1
                continue

            glyphs[codepoint] = {
                'width': width,
                'data': glyph_bytes
            }

    if debug_stripped > 0:
        print(f"  Stripped {debug_stripped} debugging glyphs from {filename}")

def generate_unifont_bin(output_path, last_codepoint):
    """Generate the unifont.bin file."""

    # Parse all Unifont hex files
    glyphs = {}
    print("Parsing Unifont HEX files...")
    parse_hex_file('unifont-12.1.03.hex', glyphs)
    parse_hex_file('unifont_upper-12.1.03.hex', glyphs)
    parse_hex_file('unifont_csur-12.1.03.hex', glyphs)

    print(f"Loaded {len(glyphs)} glyphs")

    # Filter to requested range
    glyphs = {cp: g for cp, g in glyphs.items() if cp <= last_codepoint}
    print(f"After filtering to U+{last_codepoint:04X}: {len(glyphs)} glyphs")

    # Determine plane ranges
    plane0_last = min(0xFFFF, last_codepoint)
    plane1_last = min(0x1FFFF, last_codepoint) if last_codepoint > 0xFFFF else 0
    plane2_last = min(0x2FFFF, last_codepoint) if last_codepoint > 0x1FFFF else 0

    # Calculate sizes
    header_size = 16
    plane_desc_size = 8 * 3  # 3 planes

    plane0_lut_size = (plane0_last + 1) * 4 if plane0_last > 0 else 0
    plane1_lut_size = ((plane1_last & 0xFFFF) + 1) * 4 if plane1_last > 0xFFFF else 0
    plane2_lut_size = ((plane2_last & 0xFFFF) + 1) * 4 if plane2_last > 0x1FFFF else 0

    # Calculate offsets
    plane0_lut_offset = header_size + plane_desc_size
    plane1_lut_offset = plane0_lut_offset + plane0_lut_size if plane1_lut_size > 0 else 0
    plane2_lut_offset = plane1_lut_offset + plane1_lut_size if plane2_lut_size > 0 else 0

    glyph_data_start = plane0_lut_offset + plane0_lut_size + plane1_lut_size + plane2_lut_size

    # Build glyph data and lookup tables
    glyph_data = bytearray()
    lut0 = bytearray()
    lut1 = bytearray()
    lut2 = bytearray()

    current_glyph_offset = glyph_data_start
    glyph_count = 0

    # Process Plane 0
    for cp in range(0, plane0_last + 1):
        if cp in glyphs:
            g = glyphs[cp]
            entry = current_glyph_offset | (g['width'] << 24)
            lut0 += struct.pack('<I', entry)
            glyph_data += g['data']
            current_glyph_offset += len(g['data'])
            glyph_count += 1
        else:
            lut0 += struct.pack('<I', 0)  # No glyph

    # Process Plane 1
    if plane1_last > 0xFFFF:
        for cp in range(0x10000, plane1_last + 1):
            if cp in glyphs:
                g = glyphs[cp]
                entry = current_glyph_offset | (g['width'] << 24)
                lut1 += struct.pack('<I', entry)
                glyph_data += g['data']
                current_glyph_offset += len(g['data'])
                glyph_count += 1
            else:
                lut1 += struct.pack('<I', 0)

    # Process Plane 2
    if plane2_last > 0x1FFFF:
        for cp in range(0x20000, plane2_last + 1):
            if cp in glyphs:
                g = glyphs[cp]
                entry = current_glyph_offset | (g['width'] << 24)
                lut2 += struct.pack('<I', entry)
                glyph_data += g['data']
                current_glyph_offset += len(g['data'])
                glyph_count += 1
            else:
                lut2 += struct.pack('<I', 0)

    # Build header
    header = bytearray()
    header += b'UFNT'                          # Magic
    header += struct.pack('<H', 0x0001)        # Version
    header += struct.pack('<B', 8)             # Nominal width
    header += struct.pack('<B', 16)            # Nominal height
    header += struct.pack('<I', glyph_count)   # Total glyph count
    header += struct.pack('<I', 0)             # Reserved

    # Build plane descriptors
    plane_descs = bytearray()

    # Plane 0
    plane_descs += struct.pack('<H', 0)                    # First CP
    plane_descs += struct.pack('<H', plane0_last & 0xFFFF) # Last CP
    plane_descs += struct.pack('<I', plane0_lut_offset)    # LUT offset

    # Plane 1
    if plane1_last > 0xFFFF:
        plane_descs += struct.pack('<H', 0)                     # First CP (low 16 bits)
        plane_descs += struct.pack('<H', plane1_last & 0xFFFF)  # Last CP (low 16 bits)
        plane_descs += struct.pack('<I', plane1_lut_offset)     # LUT offset
    else:
        plane_descs += struct.pack('<HHII', 0, 0, 0, 0)[:8]     # Empty plane

    # Plane 2
    if plane2_last > 0x1FFFF:
        plane_descs += struct.pack('<H', 0)                     # First CP (low 16 bits)
        plane_descs += struct.pack('<H', plane2_last & 0xFFFF)  # Last CP (low 16 bits)
        plane_descs += struct.pack('<I', plane2_lut_offset)     # LUT offset
    else:
        plane_descs += struct.pack('<HHII', 0, 0, 0, 0)[:8]     # Empty plane

    # Write output file
    output = header + plane_descs + lut0 + lut1 + lut2 + glyph_data

    with open(output_path, 'wb') as f:
        f.write(output)

    print(f"\nGenerated {output_path}")
    print(f"  Total size: {len(output):,} bytes ({len(output) / 1024 / 1024:.2f} MB)")
    print(f"  Glyph count: {glyph_count:,}")
    print(f"  Header + descriptors: {header_size + plane_desc_size} bytes")
    print(f"  Lookup tables: {len(lut0) + len(lut1) + len(lut2):,} bytes")
    print(f"  Glyph data: {len(glyph_data):,} bytes")

def main():
    parser = argparse.ArgumentParser(description='Generate unifont.bin')
    parser.add_argument('--output', '-o', default='../../system/fonts/unifont.bin',
                        help='Output file path')
    parser.add_argument('--last-codepoint', '-l', default=hex(DEFAULT_LAST_CODEPOINT),
                        help='Last codepoint to include (hex, e.g., 0x2FFFF)')
    args = parser.parse_args()

    last_cp = int(args.last_codepoint, 16) if args.last_codepoint.startswith('0x') else int(args.last_codepoint, 16)

    generate_unifont_bin(args.output, last_cp)

if __name__ == '__main__':
    main()
