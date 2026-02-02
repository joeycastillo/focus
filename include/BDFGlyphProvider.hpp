/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#pragma once

#include "GlyphProvider.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct BDFGlyph {
    uint8_t width;       // BBX width (bitmap width)
    uint8_t height;      // BBX height (original height before conversion)
    int8_t xOffset;      // BBX x offset
    int8_t yOffset;      // BBX y offset (from baseline)
    uint8_t advance;     // DWIDTH (advance width for cursor)
    std::vector<uint8_t> bitmap;  // Converted to Display format (16 or 32 bytes)
};

class BDFGlyphProvider : public GlyphProvider {
public:
    /// Construct a BDF glyph provider by loading a BDF font file
    /// @param bdfFilePath Path to the BDF font file (e.g., "/sdcard/fonts/timR12.bdf")
    BDFGlyphProvider(const std::string& bdfFilePath);

    uint8_t getPointSize() override;
    Size getMaxSize() override;
    Point getOffset() override;
    uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) override;
    Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char *font = NULL) override;

    /// Check if the font was loaded successfully
    bool isValid() const { return valid; }

    /// Get the number of glyphs loaded
    size_t getGlyphCount() const { return glyphs.size(); }

private:
    bool parseBDFFile(const std::string& path);
    void convertGlyphToDisplayFormat(BDFGlyph& glyph);
    static uint8_t hexCharToNibble(char c);
    static uint8_t hexToByte(const char* hex);

    std::unordered_map<uint32_t, BDFGlyph> glyphs;

    uint8_t pixelSize = 0;
    uint8_t fontAscent = 0;
    uint8_t fontDescent = 0;
    Size maxSize = {0, 0};
    uint32_t defaultChar = 0;
    bool valid = false;
};
