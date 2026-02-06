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

#include "UnifontGlyphProvider.hpp"
#include <cstring>

// Magic number for unifont.bin files
static const uint8_t UNIFONT_MAGIC[4] = {'U', 'F', 'N', 'T'};

// Header layout
static const size_t HEADER_SIZE = 16;
static const size_t PLANE_DESC_SIZE = 8;
static const size_t NUM_PLANES = 3;

UnifontGlyphProvider::UnifontGlyphProvider() {
    memset(glyphBuffer, 0, sizeof(glyphBuffer));
}

UnifontGlyphProvider::~UnifontGlyphProvider() {
    if (fileHandle) {
        fclose(fileHandle);
        fileHandle = nullptr;
    }
    if (ownsData && data) {
        delete[] data;
    }
}

std::shared_ptr<UnifontGlyphProvider> UnifontGlyphProvider::fromFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return nullptr;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < HEADER_SIZE + PLANE_DESC_SIZE * NUM_PLANES) {
        fclose(f);
        return nullptr;
    }

    auto provider = std::shared_ptr<UnifontGlyphProvider>(new UnifontGlyphProvider());
    provider->fileHandle = f;
    provider->dataSize = size;

    if (!provider->parseHeader()) {
        return nullptr;
    }

    return provider;
}

std::shared_ptr<UnifontGlyphProvider> UnifontGlyphProvider::fromMemory(
    const uint8_t* data, size_t size, bool ownsData)
{
    if (!data || size < HEADER_SIZE + PLANE_DESC_SIZE * NUM_PLANES) {
        return nullptr;
    }

    auto provider = std::shared_ptr<UnifontGlyphProvider>(new UnifontGlyphProvider());
    provider->data = data;
    provider->dataSize = size;
    provider->ownsData = ownsData;

    if (!provider->parseHeader()) {
        return nullptr;
    }

    return provider;
}

void UnifontGlyphProvider::readData(uint32_t offset, void* dest, size_t len) {
    if (data) {
        // Memory-mapped access
        if (offset + len <= dataSize) {
            memcpy(dest, data + offset, len);
        } else {
            memset(dest, 0, len);
        }
    } else if (fileHandle) {
        // File access
        fseek(fileHandle, offset, SEEK_SET);
        size_t read = fread(dest, 1, len, fileHandle);
        if (read < len) {
            memset((uint8_t*)dest + read, 0, len - read);
        }
    }
}

bool UnifontGlyphProvider::parseHeader() {
    uint8_t header[HEADER_SIZE];
    readData(0, header, HEADER_SIZE);

    // Check magic
    if (memcmp(header, UNIFONT_MAGIC, 4) != 0) {
        return false;
    }

    // Parse header fields
    // Bytes 4-5: version (ignored for now)
    nominalWidth = header[6];
    nominalHeight = header[7];
    glyphCount = header[8] | (header[9] << 8) | (header[10] << 16) | (header[11] << 24);

    // Parse plane descriptors
    uint8_t planeDescs[PLANE_DESC_SIZE * NUM_PLANES];
    readData(HEADER_SIZE, planeDescs, sizeof(planeDescs));

    for (size_t i = 0; i < NUM_PLANES; i++) {
        const uint8_t* pd = planeDescs + i * PLANE_DESC_SIZE;
        planes[i].firstCP = pd[0] | (pd[1] << 8);
        planes[i].lastCP = pd[2] | (pd[3] << 8);
        planes[i].lutOffset = pd[4] | (pd[5] << 8) | (pd[6] << 16) | (pd[7] << 24);
    }

    // Look up replacement character (U+FFFD) for fallback
    replacementCharOffset = lookupGlyph(0xFFFD, replacementCharWidth);

    valid = true;
    return true;
}

uint32_t UnifontGlyphProvider::lookupGlyph(UNICODE_CODEPOINT cp, uint8_t& outWidth) {
    // Determine plane
    size_t planeIdx;
    uint16_t cpLow;

    if (cp <= 0xFFFF) {
        planeIdx = 0;
        cpLow = (uint16_t)cp;
    } else if (cp <= 0x1FFFF) {
        planeIdx = 1;
        cpLow = (uint16_t)(cp - 0x10000);
    } else if (cp <= 0x2FFFF) {
        planeIdx = 2;
        cpLow = (uint16_t)(cp - 0x20000);
    } else {
        outWidth = 0;
        return 0;
    }

    const PlaneInfo& plane = planes[planeIdx];

    // Check if plane is populated and codepoint is in range
    if (plane.lutOffset == 0 || cpLow > plane.lastCP) {
        outWidth = 0;
        return 0;
    }

    // Read LUT entry (4 bytes)
    uint32_t lutAddr = plane.lutOffset + cpLow * 4;
    uint8_t entryBytes[4];
    readData(lutAddr, entryBytes, 4);

    uint32_t entry = entryBytes[0] | (entryBytes[1] << 8) |
                     (entryBytes[2] << 16) | (entryBytes[3] << 24);

    if (entry == 0) {
        outWidth = 0;
        return 0;
    }

    // Extract offset (bits 0-23) and width (bits 24-28)
    uint32_t glyphOffset = entry & 0x00FFFFFF;
    outWidth = (entry >> 24) & 0x1F;

    return glyphOffset;
}

uint8_t UnifontGlyphProvider::getPointSize() {
    return nominalHeight;  // 16 for Unifont
}

Size UnifontGlyphProvider::getMaxSize() {
    return MakeSize(16, 16);  // Maximum glyph size
}

Point UnifontGlyphProvider::getOffset() {
    return MakePoint(0, 0);
}

uint8_t UnifontGlyphProvider::getGlyphRowCount() {
    return nominalHeight;  // 16 rows
}

bool UnifontGlyphProvider::isValid() const {
    return valid;
}

uint8_t* UnifontGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
    (void)font;  // Unused

    uint8_t width;
    uint32_t offset = lookupGlyph(codepoint, width);

    if (offset == 0 || width == 0) {
        // Use replacement character
        if (replacementCharOffset != 0) {
            size_t dataLen = (replacementCharWidth <= 8) ? 16 : 32;
            readData(replacementCharOffset, glyphBuffer, dataLen);
            return glyphBuffer;
        }
        // No replacement character available, return empty
        memset(glyphBuffer, 0, sizeof(glyphBuffer));
        return glyphBuffer;
    }

    // Read glyph data
    size_t dataLen = (width <= 8) ? 16 : 32;
    readData(offset, glyphBuffer, dataLen);

    return glyphBuffer;
}

Rect UnifontGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, const char* font) {
    (void)font;  // Unused

    uint8_t width;
    uint32_t offset = lookupGlyph(codepoint, width);

    if (offset == 0 || width == 0) {
        // Use replacement character metrics
        return MakeRect(0, 0, replacementCharWidth, nominalHeight);
    }

    // Unifont glyphs have no bearing offset, origin at (0, 0)
    // Width is the advance width
    return MakeRect(0, 0, width, nominalHeight);
}
