# Unifont Binary Generator

This folder contains the script and data to generate `unifont.bin`, a compact
binary glyph file for GNU Unifont. Licensed under GPL v2+.

## Prerequisites

- Python 3.x
- Unifont HEX files (included)

## Generating unifont.bin

```bash
python3 generate_unifont.py
```

Output: `../../packedfonts/unifont.bin` (~2.8MB)

This creates a simplified glyph-only font file for Unifont with O(1) lookup.

## Source Files

- `unifont-*.hex` - Unifont glyph data (GNU GPL with font embedding exception)
