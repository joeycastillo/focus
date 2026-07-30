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

/**
 * @file UnifontGlyphProvider.hpp
 * @brief GlyphProvider implementation for Unifont bitmap font.
 *
 * Reads the simplified unifont.bin format which contains:
 * - 16-byte header with font metadata
 * - Per-plane lookup tables (4 bytes per codepoint)
 * - Densely packed glyph data (16 or 32 bytes per glyph)
 *
 * Provides O(1) glyph lookup for any Unicode codepoint in Planes 0-2.
 * Unifont glyphs are 8x16 (single-width) or 16x16 (double-width).
 */

#pragma once

#include "GlyphProvider.hpp"
#include "focus_config.h"
#include <memory>
#include <string>
#include <cstdint>
#if FOCUS_HAS_FILESYSTEM
#include <cstdio>
#endif

namespace focus {

/**
 * @brief GlyphProvider for the Unifont bitmap font.
 *
 * Unifont is a GNU project that aims to provide glyphs for every Unicode
 * codepoint. This provider reads from a simplified binary format optimized
 * for embedded systems.
 * @ingroup text
 */
class UnifontGlyphProvider : public GlyphProvider {
public:
#if FOCUS_HAS_FILESYSTEM
    /**
     * @brief Load Unifont from a file path.
     * @param path Path to unifont.bin file.
     * @return A shared_ptr to the provider, or nullptr if loading failed.
     */
    static std::shared_ptr<UnifontGlyphProvider> fromFile(const std::string& path);
#endif

    /**
     * @brief Load Unifont from a memory buffer (e.g., memory-mapped flash).
     * @param data Pointer to the font data.
     * @param size Size of the data in bytes.
     * @param ownsData If true, the provider will free the data on destruction.
     * @return A shared_ptr to the provider, or nullptr if parsing failed.
     */
    static std::shared_ptr<UnifontGlyphProvider> fromMemory(
        const uint8_t* data, size_t size, bool ownsData = false);

    ~UnifontGlyphProvider();

    // GlyphProvider interface
    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    bool isValid() const override;
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis = FontStyle::Regular) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;

private:
    UnifontGlyphProvider();

    // Data access
    const uint8_t* data = nullptr;
    size_t dataSize = 0;
    bool ownsData = false;
#if FOCUS_HAS_FILESYSTEM
    mutable FILE* fileHandle = nullptr;
#endif

    // Parsed header info
    bool valid = false;
    uint8_t nominalWidth = 8;
    uint8_t nominalHeight = 16;
    uint32_t glyphCount = 0;

    // Plane descriptors
    struct PlaneInfo {
        uint16_t firstCP = 0;
        uint16_t lastCP = 0;
        uint32_t lutOffset = 0;
    };
    PlaneInfo planes[3];

    // Glyph buffer for current lookup
    mutable uint8_t glyphBuffer[32];

    // Offset to replacement character (U+FFFD) for fallback
    uint32_t replacementCharOffset = 0;
    uint8_t replacementCharWidth = 16;

    // Internal helpers
    bool parseHeader();
    uint32_t lookupGlyph(UNICODE_CODEPOINT cp, uint8_t& outWidth) const;
    void readData(uint32_t offset, void* dest, size_t len) const;
};

}  // namespace focus
