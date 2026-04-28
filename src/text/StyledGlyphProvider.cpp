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

#include "StyledGlyphProvider.hpp"

StyledGlyphProvider::StyledGlyphProvider(
    std::shared_ptr<GlyphProvider> regular,
    std::shared_ptr<GlyphProvider> bold,
    std::shared_ptr<GlyphProvider> italic,
    std::shared_ptr<GlyphProvider> boldItalic)
{
    this->providers[0] = std::move(regular);
    this->providers[1] = std::move(italic);
    this->providers[2] = std::move(bold);
    this->providers[3] = std::move(boldItalic);
}

const GlyphProvider* StyledGlyphProvider::providerForEmphasis(uint8_t emphasis) const {
    if (emphasis < 4 && this->providers[emphasis] != nullptr) {
        return this->providers[emphasis].get();
    }
    // For bold+italic (3), try italic (1) then bold (2) before falling back to regular.
    // Italic letterforms are harder to synthesize than bold weight, so prefer real italic.
    if (emphasis == 3) {
        if (this->providers[1] != nullptr) return this->providers[1].get();
        if (this->providers[2] != nullptr) return this->providers[2].get();
    }
    return this->providers[0].get();
}

uint8_t StyledGlyphProvider::getPointSize() const {
    return this->providers[0]->getPointSize();
}

Size StyledGlyphProvider::getMaxSize() const {
    return this->providers[0]->getMaxSize();
}

Point StyledGlyphProvider::getOffset() const {
    return this->providers[0]->getOffset();
}

uint8_t StyledGlyphProvider::getGlyphRowCount() const {
    return this->providers[0]->getGlyphRowCount();
}

bool StyledGlyphProvider::isValid() const {
    return this->providers[0]->isValid();
}

std::string StyledGlyphProvider::getTitle() const {
    return this->providers[0]->getTitle();
}

const uint8_t* StyledGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis) const {
    return this->providerForEmphasis(emphasis)->glyphForCodepoint(codepoint);
}

GlyphMetrics StyledGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis) const {
    return this->providerForEmphasis(emphasis)->metricsForCodepoint(codepoint);
}

bool StyledGlyphProvider::hasGlyph(UNICODE_CODEPOINT codepoint) const {
    return this->providers[0]->hasGlyph(codepoint);
}

bool StyledGlyphProvider::supportsEmphasis(uint8_t emphasis) const {
    return emphasis < 4 && this->providers[emphasis] != nullptr;
}
