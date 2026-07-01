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
#include <cassert>

namespace focus {

StyledGlyphProvider::StyledGlyphProvider(
    std::shared_ptr<GlyphProvider> regular,
    std::shared_ptr<GlyphProvider> italic,
    std::shared_ptr<GlyphProvider> bold,
    std::shared_ptr<GlyphProvider> boldItalic)
{
    assert(regular != nullptr && "StyledGlyphProvider requires a non-null regular provider");
    this->providers[0] = std::move(regular);
    this->providers[1] = std::move(italic);
    this->providers[2] = std::move(bold);
    this->providers[3] = std::move(boldItalic);
}

const GlyphProvider* StyledGlyphProvider::providerForEmphasis(FontStyle emphasis) const {
    uint8_t index = static_cast<uint8_t>(emphasis);
    if (index < 4 && this->providers[index] != nullptr) {
        return this->providers[index].get();
    }
    // For bold+italic, try the italic variant then the bold variant before falling
    // back to regular. Italic letterforms are harder to synthesize than bold weight,
    // so prefer a real italic.
    if (emphasis == (FontStyle::Italic | FontStyle::Bold)) {
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

const uint8_t* StyledGlyphProvider::glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    return this->providerForEmphasis(emphasis)->glyphForCodepoint(codepoint);
}

GlyphMetrics StyledGlyphProvider::metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    return this->providerForEmphasis(emphasis)->metricsForCodepoint(codepoint);
}

bool StyledGlyphProvider::hasGlyph(UNICODE_CODEPOINT codepoint) const {
    return this->providers[0]->hasGlyph(codepoint);
}

bool StyledGlyphProvider::supportsEmphasis(FontStyle emphasis) const {
    uint8_t index = static_cast<uint8_t>(emphasis);
    return index < 4 && this->providers[index] != nullptr;
}

}  // namespace focus
