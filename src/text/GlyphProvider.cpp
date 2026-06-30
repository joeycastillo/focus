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

#include "GlyphProvider.hpp"

namespace focus {

GlyphProvider::GlyphProvider() {
}

const GlyphMetrics* GlyphProvider::getAsciiMetricsCache() const {
    if (!asciiCachePopulated) {
        for (int i = 0; i < 96; i++) {
            asciiMetricsCache[i] = metricsForCodepoint(0x20 + i);
        }
        asciiCachePopulated = true;
    }
    return asciiMetricsCache;
}

void GlyphProvider::convertBitmapToDisplayFormat(
    std::vector<uint8_t>& bitmap,
    uint8_t width, uint8_t height, int8_t yOffset, uint8_t advance,
    uint8_t fontAscent, uint8_t fontDescent)
{
    uint8_t renderWidth = width > advance ? width : advance;
    uint8_t destBytesPerRow = (renderWidth + 7) / 8;
    if (destBytesPerRow == 0) destBytesPerRow = 1;

    uint8_t totalRows = fontAscent + fontDescent;
    std::vector<uint8_t> converted(destBytesPerRow * totalRows, 0);

    int startRow = fontAscent - yOffset - height;
    if (startRow < 0) startRow = 0;

    uint8_t srcBytesPerRow = (width + 7) / 8;

    for (int row = 0; row < height && (startRow + row) < totalRows; row++) {
        int destRow = startRow + row;
        for (int b = 0; b < destBytesPerRow; b++) {
            size_t destIdx = destRow * destBytesPerRow + b;
            if (b < srcBytesPerRow) {
                size_t srcIdx = row * srcBytesPerRow + b;
                if (srcIdx < bitmap.size() && destIdx < converted.size()) {
                    converted[destIdx] = bitmap[srcIdx];
                }
            }
        }
    }

    bitmap = std::move(converted);
}

void GlyphProvider::applyBoldToBitmap(uint8_t* bitmap, uint8_t bytesPerRow, uint8_t rowCount) {
    for (int row = 0; row < rowCount; row++) {
        uint8_t* rowPtr = bitmap + row * bytesPerRow;
        uint8_t carry = 0;
        for (int b = 0; b < bytesPerRow; b++) {
            uint8_t orig = rowPtr[b];
            rowPtr[b] = orig | (orig >> 1) | carry;
            carry = (orig & 1) << 7;
        }
    }
}

void GlyphProvider::applyShearToBitmap(uint8_t* bitmap, uint8_t bytesPerRow, uint8_t rowCount, int shearPixels) {
    if (shearPixels <= 0 || rowCount <= 1) return;

    for (int row = 0; row < rowCount; row++) {
        int shift = shearPixels * (rowCount - 1 - row) / (rowCount - 1);
        if (shift == 0) continue;

        uint8_t* rowPtr = bitmap + row * bytesPerRow;

        // Shift the entire row right by 'shift' pixels in MSB-first bitmap.
        // Process from rightmost byte to leftmost to avoid overwriting source data.
        int byteShift = shift / 8;
        int bitShift = shift % 8;

        for (int b = bytesPerRow - 1; b >= 0; b--) {
            uint8_t val = 0;
            int srcByte = b - byteShift;
            if (srcByte >= 0) {
                val = rowPtr[srcByte] >> bitShift;
            }
            if (srcByte - 1 >= 0 && bitShift > 0) {
                val |= rowPtr[srcByte - 1] << (8 - bitShift);
            }
            rowPtr[b] = val;
        }
    }
}

}  // namespace focus
