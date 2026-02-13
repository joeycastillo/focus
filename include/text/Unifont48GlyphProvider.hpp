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

/// GlyphProvider that wraps UnifontGlyphProvider and scales glyphs to 3x
/// using the scale3xPad algorithm. Produces 24x48 or 48x48 glyphs from the
/// underlying 8x16 or 16x16 Unifont bitmaps.
class Unifont48GlyphProvider : public GlyphProvider {
public:
    static std::shared_ptr<Unifont48GlyphProvider> create(
        std::shared_ptr<UnifontGlyphProvider> base);

    uint8_t getPointSize() override;
    Size getMaxSize() override;
    Point getOffset() override;
    uint8_t getGlyphRowCount() override;
    bool isValid() const override;
    uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char* font = nullptr) override;
    Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char* font = nullptr) override;

private:
    Unifont48GlyphProvider(std::shared_ptr<UnifontGlyphProvider> base);

    std::shared_ptr<UnifontGlyphProvider> baseProvider;
    uint8_t scaledBuffer[288]; // 48 rows * 6 bytes/row (48px wide max)
};
