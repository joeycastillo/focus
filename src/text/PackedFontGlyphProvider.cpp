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

#include "PackedFontGlyphProvider.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

PackedFontGlyphProvider::PackedFontGlyphProvider(const std::string& bdpFilePath) {
    valid = loadBDPFile(bdpFilePath);
}

bool PackedFontGlyphProvider::loadBDPFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }

    // Read 16-byte header
    uint8_t header[16];
    if (fread(header, 1, 16, f) != 16) {
        fclose(f);
        return false;
    }

    // Validate magic
    if (header[0] != 'B' || header[1] != 'D' || header[2] != 'P' || header[3] != 0x01) {
        fclose(f);
        return false;
    }

    pixelSize = header[4];
    fontAscent = header[5];
    fontDescent = header[6];
    maxSize.width = header[7];
    maxSize.height = header[8];
    uint8_t titleLength = header[9];

    uint16_t numGlyphs;
    memcpy(&numGlyphs, &header[10], 2);
    memcpy(&defaultChar, &header[12], 4);

    // Read title if present
    if (titleLength > 0) {
        std::vector<char> titleBuf(titleLength);
        if (fread(titleBuf.data(), 1, titleLength, f) != titleLength) {
            fclose(f);
            return false;
        }
        title.assign(titleBuf.data(), titleLength);
    }

    // Read glyph table (9 bytes per entry)
    struct GlyphEntry {
        uint32_t codepoint;
        uint8_t advance;
        uint8_t width;
        uint8_t height;
        int8_t xOffset;
        int8_t yOffset;
    };

    std::vector<GlyphEntry> entries(numGlyphs);
    for (uint16_t i = 0; i < numGlyphs; i++) {
        uint8_t buf[9];
        if (fread(buf, 1, 9, f) != 9) {
            fclose(f);
            return false;
        }
        memcpy(&entries[i].codepoint, buf, 4);
        entries[i].advance = buf[4];
        entries[i].width = buf[5];
        entries[i].height = buf[6];
        entries[i].xOffset = static_cast<int8_t>(buf[7]);
        entries[i].yOffset = static_cast<int8_t>(buf[8]);
    }

    // Read bitmap data (sequential, same order as glyph table)
    for (uint16_t i = 0; i < numGlyphs; i++) {
        size_t bitmapSize = ((entries[i].width + 7) / 8) * entries[i].height;

        BDPGlyph glyph;
        glyph.width = entries[i].width;
        glyph.height = entries[i].height;
        glyph.xOffset = entries[i].xOffset;
        glyph.yOffset = entries[i].yOffset;
        glyph.advance = entries[i].advance;
        glyph.bitmap.resize(bitmapSize);

        if (bitmapSize > 0) {
            if (fread(glyph.bitmap.data(), 1, bitmapSize, f) != bitmapSize) {
                fclose(f);
                return false;
            }
        }

        convertGlyphToDisplayFormat(glyph);
        glyphs[entries[i].codepoint] = std::move(glyph);
    }

    fclose(f);
    return !glyphs.empty();
}

void PackedFontGlyphProvider::convertGlyphToDisplayFormat(BDPGlyph& glyph) {
    // Same logic as BDFGlyphProvider::convertGlyphToDisplayFormat
    uint8_t destBytesPerRow = (glyph.advance + 7) / 8;
    if (destBytesPerRow == 0) destBytesPerRow = 1;

    uint8_t totalRows = fontAscent + fontDescent;
    std::vector<uint8_t> converted(destBytesPerRow * totalRows, 0);

    int startRow = fontAscent - glyph.yOffset - glyph.height;
    if (startRow < 0) startRow = 0;

    uint8_t srcBytesPerRow = (glyph.width + 7) / 8;

    for (int row = 0; row < glyph.height && (startRow + row) < totalRows; row++) {
        int destRow = startRow + row;

        for (int b = 0; b < destBytesPerRow; b++) {
            size_t destIdx = destRow * destBytesPerRow + b;

            if (b < srcBytesPerRow) {
                size_t srcIdx = row * srcBytesPerRow + b;
                if (srcIdx < glyph.bitmap.size() && destIdx < converted.size()) {
                    converted[destIdx] = glyph.bitmap[srcIdx];
                }
            }
        }
    }

    glyph.bitmap = std::move(converted);
}

uint8_t PackedFontGlyphProvider::getPointSize() const {
    return pixelSize;
}

Size PackedFontGlyphProvider::getMaxSize() const {
    return maxSize;
}

Point PackedFontGlyphProvider::getOffset() const {
    return MakePoint(0, -static_cast<int>(fontAscent));
}

uint8_t PackedFontGlyphProvider::getGlyphRowCount() const {
    return fontAscent + fontDescent;
}

const uint8_t* PackedFontGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint) const {
    auto it = glyphs.find(codepoint);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    it = glyphs.find(defaultChar);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    it = glyphs.find(32);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    return nullptr;
}

Rect PackedFontGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint) const {
    auto it = glyphs.find(codepoint);
    if (it == glyphs.end()) {
        it = glyphs.find(defaultChar);
        if (it == glyphs.end()) {
            it = glyphs.find(32);
            if (it == glyphs.end()) {
                return RectZero;
            }
        }
    }

    const BDPGlyph& glyph = it->second;
    return MakeRect(glyph.xOffset, glyph.yOffset, glyph.advance, glyph.height);
}

std::string PackedFontGlyphProvider::readTitle(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";

    uint8_t header[16];
    if (fread(header, 1, 16, f) != 16) {
        fclose(f);
        return "";
    }

    if (header[0] != 'B' || header[1] != 'D' || header[2] != 'P' || header[3] != 0x01) {
        fclose(f);
        return "";
    }

    uint8_t titleLength = header[9];
    if (titleLength == 0) {
        fclose(f);
        return "";
    }

    std::vector<char> buf(titleLength);
    if (fread(buf.data(), 1, titleLength, f) != titleLength) {
        fclose(f);
        return "";
    }

    fclose(f);
    return std::string(buf.data(), titleLength);
}
