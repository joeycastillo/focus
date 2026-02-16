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
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

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
                int val;
                iss >> val;
                pixelSize = static_cast<uint8_t>(val);
            } else if (keyword == "FONT_ASCENT") {
                int val;
                iss >> val;
                fontAscent = static_cast<uint8_t>(val);
            } else if (keyword == "FONT_DESCENT") {
                int val;
                iss >> val;
                fontDescent = static_cast<uint8_t>(val);
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
    // Display expects fixed-height glyphs based on fontAscent + fontDescent
    // IMPORTANT: bytesPerRow must be based on ADVANCE width, not bitmap width!
    // Display::drawGlyph uses metricsForCodepoint().size.width to calculate bytesPerRow,
    // and we return advance as the width. So the bitmap must be sized for advance width.
    uint8_t destBytesPerRow = (glyph.advance + 7) / 8;
    if (destBytesPerRow == 0) destBytesPerRow = 1; // Minimum 1 byte per row

    uint8_t totalRows = fontAscent + fontDescent;
    std::vector<uint8_t> converted(destBytesPerRow * totalRows, 0);

    // Calculate where this glyph sits within the totalRows space
    // fontAscent is the baseline position from top
    // yOffset is the glyph's offset from baseline (positive = above baseline)
    // glyph.height is the number of rows in the glyph
    //
    // Example: fontAscent=20, yOffset=0, height=11 for lowercase 'x'
    // The glyph's bottom is at baseline, top is at row (20 - 11) = 9
    //
    // For descenders like 'g' with yOffset=-4:
    // Bottom is 4 pixels below baseline, top is at row (20 - (-4) - height) = (20 + 4 - height)
    int startRow = fontAscent - glyph.yOffset - glyph.height;
    if (startRow < 0) startRow = 0;

    uint8_t srcBytesPerRow = (glyph.width + 7) / 8;

    for (int row = 0; row < glyph.height && (startRow + row) < totalRows; row++) {
        int destRow = startRow + row;

        // Copy source bytes, padding with zeros if dest is wider
        for (int b = 0; b < destBytesPerRow; b++) {
            size_t destIdx = destRow * destBytesPerRow + b;

            if (b < srcBytesPerRow) {
                size_t srcIdx = row * srcBytesPerRow + b;
                if (srcIdx < glyph.bitmap.size() && destIdx < converted.size()) {
                    converted[destIdx] = glyph.bitmap[srcIdx];
                }
            }
            // else: dest byte remains 0 (padding)
        }
    }

    glyph.bitmap = std::move(converted);
}

uint8_t BDFGlyphProvider::getPointSize() {
    return pixelSize;
}

Size BDFGlyphProvider::getMaxSize() {
    return maxSize;
}

Point BDFGlyphProvider::getOffset() {
    // Return baseline offset for proper text positioning
    return MakePoint(0, -static_cast<int>(fontAscent));
}

uint8_t BDFGlyphProvider::getGlyphRowCount() {
    return fontAscent + fontDescent;
}

uint8_t* BDFGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
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

Rect BDFGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
    auto it = glyphs.find(codepoint);
    if (it == glyphs.end()) {
        // Try default character
        it = glyphs.find(defaultChar);
        if (it == glyphs.end()) {
            // Try space
            it = glyphs.find(32);
            if (it == glyphs.end()) {
                return RectZero;
            }
        }
    }

    const BDFGlyph& glyph = it->second;

    // Return metrics compatible with TextLayout expectations
    // width field is used for cursor advancement, so return advance
    return MakeRect(glyph.xOffset, glyph.yOffset, glyph.advance, glyph.height);
}
