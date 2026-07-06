# BDF → BDP Converter

Converts a BDF bitmap font to BDP ("Bitmap Distribution Packed"), a binary format imagined for Focus, and the format that `PackedFontGlyphProvider` loads.

## Usage

```bash
python3 bdf_to_bdp.py input.bdf output.bdp --title "Display Name"
```

One BDF in; one BDP out. `--title` is optional; it packs a human-readable name into the file for a font picker to show. The functions `parse_bdf` and `write_bdp` are importable for batch tools.

## BDP format

All multi-byte fields are little-endian. Every field but the magic, the length prefixes, and the title is a binary transcription of the like-named BDF property (the **BDF** column). `PackedFontGlyphProvider` is the reference reader.

### Header (16 bytes)

| Offset | Size | Field | BDF | Notes |
|-------:|-----:|-------|-----|-------|
| 0 | 4 | magic | — | `42 44 50 01` — `"BDP\x01"` |
| 4 | 1 | pixelSize | `PIXEL_SIZE` | nominal design size; informational |
| 5 | 1 | fontAscent | `FONT_ASCENT` | pixels above the baseline |
| 6 | 1 | fontDescent | `FONT_DESCENT` | pixels below the baseline |
| 7 | 1 | maxWidth | `FONTBOUNDINGBOX` w | max glyph bounding box |
| 8 | 1 | maxHeight | `FONTBOUNDINGBOX` h | max glyph bounding box |
| 9 | 1 | titleLength | — | title byte count; 0 = no title |
| 10 | 2 | numGlyphs | — | number of encoded glyphs written (BDF `CHARS` minus any unencoded characters) |
| 12 | 4 | defaultChar | `DEFAULT_CHAR` | codepoint to be drawn for a missing glyph |

### Title

After the header, `titleLength` bytes of UTF-8 text appear, with no null terminator, present only when `titleLength > 0`. This is the one field with no BDF source — it comes from the packer's `--title` argument — and serves as a display name for any UI that interfaces with the font.

### Glyph table

After the title, `numGlyphs` entries of 9 bytes each appear, ascending by codepoint:

| Offset | Size | Field | BDF | Notes |
|-------:|-----:|-------|-----|-------|
| 0 | 4 | codepoint | `ENCODING` | Unicode scalar value |
| 4 | 1 | advance | `DWIDTH` x | pen advance, pixels |
| 5 | 1 | width | `BBX` w | glyph bitmap width |
| 6 | 1 | height | `BBX` h | glyph bitmap height |
| 7 | 1 | xOffset | `BBX` x-off | signed; left edge from the pen origin |
| 8 | 1 | yOffset | `BBX` y-off | signed; bottom edge from the baseline |

### Bitmap data

After the Glyph table comes the bitmap data. For each glyph, in glyph-table order, we include the BDF `BITMAP` hex data trimmed to the glyph's width: `ceil(width / 8) * height` bytes, row-major, MSB first. A set bit is a set pixel; rows are padded to a byte (a glyph 8 pixels wide occupies one byte per row; a glyph 9 pixels wide occupies two).

Note that a glyph's bitmap data is NOT interleaved with the glyph table. The glyph table and bitmap blobs are parallel arrays joined by order: a glyph's bitmap data offset is the sum of the preceding glyphs' byte sizes, and _no offset into the table is stored_. A reader that wants random access can build that index once at load.
