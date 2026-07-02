/*
 * Fixed-width mock GlyphProvider for unit tests.
 *
 * Every glyph has the same advance width (default 8px at textSize=1).
 * This makes width calculations deterministic and predictable:
 *   10 characters at width=8 → 80px total.
 */

#pragma once

#include "GlyphProvider.hpp"
#include <cstring>
#include <set>

class MockGlyphProvider : public focus::GlyphProvider {
public:
    explicit MockGlyphProvider(int16_t glyphWidth = 8, uint8_t rowCount = 12)
        : yOffset(0), glyphWidth(glyphWidth), rowCount(rowCount) {
        // Zero-fill the bitmap
        memset(bitmap, 0, sizeof(bitmap));
    }

    uint8_t getPointSize() const override { return this->rowCount; }
    focus::Size getMaxSize() const override { return {this->glyphWidth, this->rowCount}; }
    focus::Point getOffset() const override { return {0, static_cast<int16_t>(-this->yOffset)}; }
    uint8_t getGlyphRowCount() const override { return this->rowCount; }
    bool isValid() const override { return true; }
    std::string getTitle() const override { return "MockFont"; }

    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint, focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        return this->bitmap;
    }

    focus::GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, focus::FontStyle emphasis = focus::FontStyle::Regular) const override {
        return focus::GlyphMetrics{static_cast<uint8_t>(this->glyphWidth), static_cast<uint8_t>(this->glyphWidth), this->rowCount, 0, 0};
    }

    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override {
        if (this->limitedGlyphSet.empty()) return true;
        return this->limitedGlyphSet.count(codepoint) > 0;
    }

    std::set<UNICODE_CODEPOINT> limitedGlyphSet;
    uint8_t bitmap[32 * 6] = {};  // large enough for any reasonable glyph (48px wide * 32 rows)
    int16_t yOffset;              // ascent value (getOffset returns {0, -yOffset})

private:
    int16_t glyphWidth;
    uint8_t rowCount;
};
