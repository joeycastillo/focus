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

#pragma once

#include "GlyphProvider.hpp"
#include "UnifontGlyphProvider.hpp"
#include <memory>

/// GlyphProvider that wraps UnifontGlyphProvider and scales glyphs to 2x
/// using the hq2x algorithm. Produces 16x32 or 32x32 glyphs from the
/// underlying 8x16 or 16x16 Unifont bitmaps.
class Unifont32GlyphProvider : public GlyphProvider {
public:
    static std::shared_ptr<Unifont32GlyphProvider> create(
        std::shared_ptr<UnifontGlyphProvider> base);

    uint8_t getPointSize() override;
    Size getMaxSize() override;
    Point getOffset() override;
    uint8_t getGlyphRowCount() override;
    bool isValid() const override;
    uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char* font = nullptr) override;
    Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char* font = nullptr) override;

private:
    Unifont32GlyphProvider(std::shared_ptr<UnifontGlyphProvider> base);

    std::shared_ptr<UnifontGlyphProvider> baseProvider;
    uint8_t scaledBuffer[128]; // 32 rows * 4 bytes/row (32px wide max)
};
