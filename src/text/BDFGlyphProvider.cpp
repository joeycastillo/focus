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

#include "BDFGlyphProvider.hpp"
#include "FocusLog.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace focus {

static const char *TAG = "BDF";

BDFGlyphProvider::BDFGlyphProvider(const std::string& bdfFilePath) {
    valid = parseBDFFile(bdfFilePath);
}

uint8_t BDFGlyphProvider::hexCharToNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

uint8_t BDFGlyphProvider::hexToByte(const char* hex) {
    return (hexCharToNibble(hex[0]) << 4) | hexCharToNibble(hex[1]);
}

bool BDFGlyphProvider::parseBDFFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    bool inChar = false;
    bool inBitmap = false;

    // Current character being parsed
    uint32_t currentEncoding = 0;
    BDFGlyph currentGlyph;
    std::vector<uint8_t> rawBitmap;

    while (std::getline(file, line)) {
        // Trim trailing whitespace/CR
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (!inChar) {
            // Header parsing
            if (keyword == "PIXEL_SIZE") {
                int value;
                iss >> value;
                pixelSize = static_cast<uint8_t>(value);
            } else if (keyword == "FONT_ASCENT") {
                int value;
                iss >> value;
                if (value > 255) {
                    FOCUS_LOGW(TAG, "FONT_ASCENT %d exceeds uint8_t, clamped to 255", value);
                    value = 255;
                } else if (value < 0) {
                    value = 0;
                }
                fontAscent = static_cast<uint8_t>(value);
            } else if (keyword == "FONT_DESCENT") {
                int value;
                iss >> value;
                if (value > 255) {
                    FOCUS_LOGW(TAG, "FONT_DESCENT %d exceeds uint8_t, clamped to 255", value);
                    value = 255;
                } else if (value < 0) {
                    value = 0;
                }
                fontDescent = static_cast<uint8_t>(value);
            } else if (keyword == "FONTBOUNDINGBOX") {
                int w, h, xoff, yoff;
                iss >> w >> h >> xoff >> yoff;
                maxSize.width = w;
                maxSize.height = h;
            } else if (keyword == "DEFAULT_CHAR") {
                iss >> defaultChar;
            } else if (keyword == "STARTCHAR") {
                inChar = true;
                currentGlyph = BDFGlyph();
                rawBitmap.clear();
            }
        } else {
            // Character parsing
            if (keyword == "ENCODING") {
                iss >> currentEncoding;
            } else if (keyword == "DWIDTH") {
                int advance, unused;
                iss >> advance >> unused;
                currentGlyph.advance = static_cast<uint8_t>(advance);
            } else if (keyword == "BBX") {
                int w, h, xoff, yoff;
                iss >> w >> h >> xoff >> yoff;
                currentGlyph.width = static_cast<uint8_t>(w);
                currentGlyph.height = static_cast<uint8_t>(h);
                currentGlyph.xOffset = static_cast<int8_t>(xoff);
                currentGlyph.yOffset = static_cast<int8_t>(yoff);
            } else if (keyword == "BITMAP") {
                inBitmap = true;
            } else if (keyword == "ENDCHAR") {
                // Convert raw bitmap to glyph format
                uint8_t bytesPerRow = (currentGlyph.width + 7) / 8;
                currentGlyph.bitmap.resize(bytesPerRow * currentGlyph.height);

                // Copy raw bitmap data
                size_t copySize = std::min(rawBitmap.size(), currentGlyph.bitmap.size());
                std::memcpy(currentGlyph.bitmap.data(), rawBitmap.data(), copySize);

                // Convert to Display format (16 rows)
                convertGlyphToDisplayFormat(currentGlyph);

                // Store the glyph
                glyphs[currentEncoding] = std::move(currentGlyph);

                inChar = false;
                inBitmap = false;
            } else if (inBitmap) {
                // Parse bitmap hex line
                // Each line is hex data, e.g., "F0" or "FE00"
                const char* hexData = line.c_str();
                size_t hexLen = line.length();

                // Parse pairs of hex characters
                for (size_t i = 0; i + 1 < hexLen; i += 2) {
                    rawBitmap.push_back(hexToByte(hexData + i));
                }
            }
        }
    }

    file.close();

    // Ensure we have at least some glyphs
    return !glyphs.empty();
}

void BDFGlyphProvider::convertGlyphToDisplayFormat(BDFGlyph& glyph) {
    convertBitmapToDisplayFormat(
        glyph.bitmap, glyph.width, glyph.height, glyph.yOffset, glyph.advance,
        this->fontAscent, this->fontDescent);
}

uint8_t BDFGlyphProvider::getPointSize() const {
    return pixelSize;
}

Size BDFGlyphProvider::getMaxSize() const {
    return maxSize;
}

Point BDFGlyphProvider::getOffset() const {
    // Return baseline offset for proper text positioning
    return MakePoint(0, -static_cast<int>(fontAscent));
}

uint8_t BDFGlyphProvider::getGlyphRowCount() const {
    return fontAscent + fontDescent;
}

const uint8_t* BDFGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    auto it = glyphs.find(codepoint);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    // Fall back to default character
    it = glyphs.find(defaultChar);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    // Fall back to space
    it = glyphs.find(32);
    if (it != glyphs.end()) {
        return it->second.bitmap.data();
    }

    return nullptr;
}

bool BDFGlyphProvider::hasGlyph(UNICODE_CODEPOINT codepoint) const {
    return this->glyphs.find(codepoint) != this->glyphs.end();
}

GlyphMetrics BDFGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    auto it = glyphs.find(codepoint);
    if (it == glyphs.end()) {
        // Try default character
        it = glyphs.find(defaultChar);
        if (it == glyphs.end()) {
            // Try space
            it = glyphs.find(32);
            if (it == glyphs.end()) {
                return GlyphMetrics{};
            }
        }
    }

    const BDFGlyph& glyph = it->second;

    return GlyphMetrics{glyph.advance, glyph.width, glyph.height, glyph.xOffset, 0};
}

}  // namespace focus
