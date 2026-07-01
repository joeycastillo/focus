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

#include "FallbackGlyphProvider.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace focus {

FallbackGlyphProvider::FallbackGlyphProvider(
    std::shared_ptr<GlyphProvider> primary,
    std::shared_ptr<GlyphProvider> fallback,
    uint8_t fallbackAscent)
    : primary(std::move(primary))
    , fallback(std::move(fallback))
    , fallbackAscent(fallbackAscent)
{
    assert(this->primary != nullptr && "FallbackGlyphProvider requires a non-null primary provider");
    assert(this->fallback != nullptr && "FallbackGlyphProvider requires a non-null fallback provider");
    // Pre-allocate enough for 48px wide * 48 rows (generous for any reasonable glyph).
    this->glyphBuffer.resize(288, 0);
}

uint8_t FallbackGlyphProvider::getPointSize() const {
    return this->primary->getPointSize();
}

Size FallbackGlyphProvider::getMaxSize() const {
    return this->primary->getMaxSize();
}

Point FallbackGlyphProvider::getOffset() const {
    return this->primary->getOffset();
}

uint8_t FallbackGlyphProvider::getGlyphRowCount() const {
    return this->primary->getGlyphRowCount();
}

bool FallbackGlyphProvider::isValid() const {
    return this->primary->isValid();
}

std::string FallbackGlyphProvider::getTitle() const {
    return this->primary->getTitle();
}

bool FallbackGlyphProvider::supportsEmphasis(FontStyle emphasis) const {
    return this->primary->supportsEmphasis(emphasis);
}

bool FallbackGlyphProvider::hasGlyph(UNICODE_CODEPOINT codepoint) const {
    return this->primary->hasGlyph(codepoint);
}

GlyphMetrics FallbackGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    if (this->primary->hasGlyph(codepoint)) {
        return this->primary->metricsForCodepoint(codepoint, emphasis);
    }

    GlyphMetrics fbMetrics = this->fallback->metricsForCodepoint(codepoint);
    uint8_t primaryRowCount = this->primary->getGlyphRowCount();
    uint8_t fallbackRowCount = this->fallback->getGlyphRowCount();
    fbMetrics.height = std::max(primaryRowCount, fallbackRowCount);

    // Adjust bitmapWidth for synthetic emphasis applied by reformatFallbackGlyph
    int extraWidth = 0;
    if (contains(emphasis, FontStyle::Bold) && this->primary->supportsEmphasis(FontStyle::Bold)) {
        extraWidth += 1;
    }
    if (contains(emphasis, FontStyle::Italic) && this->primary->supportsEmphasis(FontStyle::Italic)) {
        extraWidth += fbMetrics.height / 4; // shear amount
    }
    if (extraWidth > 0) {
        fbMetrics.bitmapWidth += extraWidth;
    }

    // Bold adds 1px to advance when the primary supports bold
    bool wantBold = contains(emphasis, FontStyle::Bold);
    if (wantBold && this->primary->supportsEmphasis(FontStyle::Bold)) {
        fbMetrics.advance += 1;
    }

    // Vertical render adjustment: when the fallback has more ascent than the
    // primary, shift the glyph upward so baselines align. The renderer applies
    // this as an offset to the y draw position.
    int primaryAscent = -this->primary->getOffset().y;
    if (this->fallbackAscent > primaryAscent) {
        fbMetrics.yOffset = static_cast<int8_t>(primaryAscent - this->fallbackAscent);
    }

    return fbMetrics;
}

const uint8_t* FallbackGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    if (this->primary->hasGlyph(codepoint)) {
        return this->primary->glyphForCodepoint(codepoint, emphasis);
    }

    this->reformatFallbackGlyph(codepoint, emphasis);
    return this->glyphBuffer.data();
}

void FallbackGlyphProvider::reformatFallbackGlyph(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    // 1. Get fallback metrics and glyph data
    GlyphMetrics fbMetrics = this->fallback->metricsForCodepoint(codepoint);
    const uint8_t* fbGlyph = this->fallback->glyphForCodepoint(codepoint);

    uint8_t primaryRowCount = this->primary->getGlyphRowCount();
    uint8_t fallbackRowCount = this->fallback->getGlyphRowCount();

    // 2. Compute output dimensions
    uint8_t outputRowCount = std::max(primaryRowCount, fallbackRowCount);
    uint8_t fallbackWidth = fbMetrics.bitmapWidth;
    uint8_t extraWidth = 0;

    bool wantBold = contains(emphasis, FontStyle::Bold);
    bool wantItalic = contains(emphasis, FontStyle::Italic);
    bool primarySupportsBold = this->primary->supportsEmphasis(FontStyle::Bold);
    bool primarySupportsItalic = this->primary->supportsEmphasis(FontStyle::Italic);

    int shearPixels = 0;
    if (wantItalic && primarySupportsItalic) {
        shearPixels = outputRowCount / 4;
        extraWidth += shearPixels;
    }
    if (wantBold && primarySupportsBold) {
        extraWidth += 1;
    }

    uint8_t outputWidth = fallbackWidth + extraWidth;
    uint8_t outputBytesPerRow = (outputWidth + 7) / 8;
    if (outputBytesPerRow == 0) outputBytesPerRow = 1;

    // 3. Ensure buffer is large enough and clear it
    size_t bufferSize = static_cast<size_t>(outputBytesPerRow) * outputRowCount;
    if (this->glyphBuffer.size() < bufferSize) {
        this->glyphBuffer.resize(bufferSize);
    }
    std::memset(this->glyphBuffer.data(), 0, bufferSize);

    // 4. Compute vertical position for baseline alignment
    int primaryAscent = -this->primary->getOffset().y;
    int startRow = 0;
    if (fallbackRowCount <= primaryRowCount) {
        if (this->fallbackAscent > 0) {
            startRow = primaryAscent - this->fallbackAscent;
        } else {
            startRow = primaryAscent - fallbackRowCount;
        }
        if (startRow < 0) startRow = 0;
    }
    // If fallback is taller than primary, start at row 0 (baselines naturally align)

    // 5. Copy fallback bitmap rows into glyphBuffer
    uint8_t fbBytesPerRow = (fallbackWidth + 7) / 8;
    if (fbGlyph != nullptr) {
        for (int row = 0; row < fallbackRowCount && (startRow + row) < outputRowCount; row++) {
            int destRow = startRow + row;
            for (int b = 0; b < fbBytesPerRow && b < outputBytesPerRow; b++) {
                this->glyphBuffer[destRow * outputBytesPerRow + b] = fbGlyph[row * fbBytesPerRow + b];
            }
        }
    }

    // 6. Apply shear (italic) if primary supports italic and emphasis requests it
    if (wantItalic && primarySupportsItalic && shearPixels > 0) {
        applyShearToBitmap(this->glyphBuffer.data(), outputBytesPerRow, outputRowCount, shearPixels);
    }

    // 7. Apply bold if primary supports bold and emphasis requests it
    if (wantBold && primarySupportsBold) {
        applyBoldToBitmap(this->glyphBuffer.data(), outputBytesPerRow, outputRowCount);
    }
}

}  // namespace focus
