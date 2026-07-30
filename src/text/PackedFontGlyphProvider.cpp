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
#include "focus_config.h"
#include <cstring>
#include <algorithm>
#if FOCUS_HAS_FILESYSTEM
#include <cstdio>
#endif

namespace focus {

PackedFontGlyphProvider::PackedFontGlyphProvider(const std::string& bdpFilePath) {
#if FOCUS_HAS_FILESYSTEM
    valid = loadBDPFile(bdpFilePath);
#else
    (void)bdpFilePath;
    valid = false;
#endif
}

std::shared_ptr<PackedFontGlyphProvider>
PackedFontGlyphProvider::fromMemory(const uint8_t* data, size_t size) {
    auto provider = std::shared_ptr<PackedFontGlyphProvider>(new PackedFontGlyphProvider());
    provider->valid = provider->loadBDP(data, size);
    if (!provider->valid) return nullptr;
    return provider;
}

bool PackedFontGlyphProvider::loadBDP(const uint8_t* data, size_t size) {
    if (!data || size < 16) return false;

    if (data[0] != 'B' || data[1] != 'D' || data[2] != 'P' || data[3] != 0x01) {
        return false;
    }

    pixelSize = data[4];
    fontAscent = data[5];
    fontDescent = data[6];
    maxSize.width = data[7];
    maxSize.height = data[8];
    uint8_t titleLength = data[9];

    uint16_t numGlyphs;
    memcpy(&numGlyphs, &data[10], 2);
    memcpy(&defaultChar, &data[12], 4);

    size_t pos = 16;

    if (pos + titleLength > size) return false;
    if (titleLength > 0) {
        title.assign(reinterpret_cast<const char*>(data + pos), titleLength);
        pos += titleLength;
    }

    struct GlyphEntry {
        uint32_t codepoint;
        uint8_t advance, width, height;
        int8_t xOffset, yOffset;
    };

    std::vector<GlyphEntry> entries(numGlyphs);
    for (uint16_t i = 0; i < numGlyphs; ++i) {
        if (pos + 9 > size) return false;
        const uint8_t* b = data + pos;
        memcpy(&entries[i].codepoint, b, 4);
        entries[i].advance = b[4];
        entries[i].width   = b[5];
        entries[i].height  = b[6];
        entries[i].xOffset = static_cast<int8_t>(b[7]);
        entries[i].yOffset = static_cast<int8_t>(b[8]);
        pos += 9;
    }

    for (uint16_t i = 0; i < numGlyphs; ++i) {
        size_t bitmapSize = ((entries[i].width + 7) / 8) * entries[i].height;

        BDPGlyph glyph;
        glyph.width = entries[i].width;
        glyph.height = entries[i].height;
        glyph.xOffset = entries[i].xOffset;
        glyph.yOffset = entries[i].yOffset;
        glyph.advance = entries[i].advance;
        glyph.bitmap.resize(bitmapSize);

        if (bitmapSize > 0) {
            if (pos + bitmapSize > size) return false;
            memcpy(glyph.bitmap.data(), data + pos, bitmapSize);
            pos += bitmapSize;
        }

        convertGlyphToDisplayFormat(glyph);
        glyphs[entries[i].codepoint] = std::move(glyph);
    }

    return !glyphs.empty();
}

void PackedFontGlyphProvider::convertGlyphToDisplayFormat(BDPGlyph& glyph) {
    convertBitmapToDisplayFormat(
        glyph.bitmap, glyph.width, glyph.height, glyph.yOffset, glyph.advance,
        this->fontAscent, this->fontDescent);
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

const uint8_t* PackedFontGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
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

bool PackedFontGlyphProvider::hasGlyph(UNICODE_CODEPOINT codepoint) const {
    return this->glyphs.find(codepoint) != this->glyphs.end();
}

GlyphMetrics PackedFontGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    auto it = glyphs.find(codepoint);
    if (it == glyphs.end()) {
        it = glyphs.find(defaultChar);
        if (it == glyphs.end()) {
            it = glyphs.find(32);
            if (it == glyphs.end()) {
                return GlyphMetrics{};
            }
        }
    }

    const BDPGlyph& glyph = it->second;
    return GlyphMetrics{glyph.advance, glyph.width, glyph.height, glyph.xOffset, 0};
}

#if FOCUS_HAS_FILESYSTEM
bool PackedFontGlyphProvider::loadBDPFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    size_t rd = fread(buf.data(), 1, static_cast<size_t>(sz), f);
    fclose(f);
    if (rd != static_cast<size_t>(sz)) return false;
    return loadBDP(buf.data(), buf.size());
}

std::string PackedFontGlyphProvider::readTitle(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";
    uint8_t header[16];
    if (fread(header, 1, 16, f) != 16) { fclose(f); return ""; }
    if (header[0] != 'B' || header[1] != 'D' || header[2] != 'P' || header[3] != 0x01) {
        fclose(f);
        return "";
    }
    uint8_t titleLength = header[9];
    if (titleLength == 0) { fclose(f); return ""; }
    std::vector<char> buf(titleLength);
    if (fread(buf.data(), 1, titleLength, f) != titleLength) { fclose(f); return ""; }
    fclose(f);
    return std::string(buf.data(), titleLength);
}
#endif  // FOCUS_HAS_FILESYSTEM

}  // namespace focus
