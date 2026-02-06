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

#include "Unifont32GlyphProvider.hpp"
#include <hqx/HQ2x.hh>
#include <cstring>

static const uint32_t BLACK = 0xFF000000;
static const uint32_t WHITE = 0xFFFFFFFF;

Unifont32GlyphProvider::Unifont32GlyphProvider(std::shared_ptr<UnifontGlyphProvider> base)
    : baseProvider(std::move(base))
{
    memset(scaledBuffer, 0, sizeof(scaledBuffer));
}

std::shared_ptr<Unifont32GlyphProvider> Unifont32GlyphProvider::create(
    std::shared_ptr<UnifontGlyphProvider> base)
{
    if (!base || !base->isValid()) return nullptr;
    return std::shared_ptr<Unifont32GlyphProvider>(new Unifont32GlyphProvider(std::move(base)));
}

uint8_t Unifont32GlyphProvider::getPointSize() {
    return 32;
}

Size Unifont32GlyphProvider::getMaxSize() {
    return MakeSize(32, 32);
}

Point Unifont32GlyphProvider::getOffset() {
    return MakePoint(0, 0);
}

uint8_t Unifont32GlyphProvider::getGlyphRowCount() {
    return 32;
}

bool Unifont32GlyphProvider::isValid() const {
    return baseProvider && baseProvider->isValid();
}

Rect Unifont32GlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
    Rect base = baseProvider->metricsForCodepoint(codepoint, font);
    return MakeRect(base.origin.x * 2, base.origin.y * 2,
                    base.size.width * 2, base.size.height * 2);
}

uint8_t* Unifont32GlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
    Rect baseMetrics = baseProvider->metricsForCodepoint(codepoint, font);
    uint8_t* baseGlyph = baseProvider->glyphForCodepoint(codepoint, font);
    int baseWidth = baseMetrics.size.width; // 8 or 16
    int baseHeight = 16;
    int baseBytesPerRow = (baseWidth + 7) / 8; // 1 or 2

    // Unpack 1-bit bitmap to ARGB pixels
    uint32_t input[16 * 16]; // max 16x16
    for (int row = 0; row < baseHeight; row++) {
        for (int col = 0; col < baseWidth; col++) {
            int byteIdx = col / 8;
            int bitIdx = 7 - (col % 8);
            bool set = (baseGlyph[row * baseBytesPerRow + byteIdx] >> bitIdx) & 1;
            input[row * baseWidth + col] = set ? BLACK : WHITE;
        }
    }

    // Run hq2x: produces (baseWidth*2) x (baseHeight*2) output
    int outWidth = baseWidth * 2;
    int outHeight = baseHeight * 2;
    uint32_t output[32 * 32]; // max 32x32
    HQ2x scaler;
    scaler.resize(input, baseWidth, baseHeight, output);

    // Pack ARGB output back to 1-bit MSB-first bitmap
    int outBytesPerRow = (outWidth + 7) / 8;
    memset(scaledBuffer, 0, sizeof(scaledBuffer));
    for (int row = 0; row < outHeight; row++) {
        for (int col = 0; col < outWidth; col++) {
            uint32_t pixel = output[row * outWidth + col];
            // Extract green channel as luminance proxy, threshold at 128
            uint8_t lum = (pixel >> 8) & 0xFF;
            if (lum < 128) {
                // Set bit (black pixel)
                scaledBuffer[row * outBytesPerRow + col / 8] |= (1 << (7 - (col % 8)));
            }
        }
    }

    return scaledBuffer;
}
