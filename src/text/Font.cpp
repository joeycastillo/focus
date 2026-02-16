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

#include "Font.hpp"
#include "PackedFontGlyphProvider.hpp"
#include "BDFGlyphProvider.hpp"

// Static member initialization
std::map<std::string, std::shared_ptr<Font>> Font::fontCache;
std::vector<std::string> Font::searchPaths;
std::shared_ptr<Font> Font::defaultSystemFont = nullptr;
std::shared_ptr<Font> Font::defaultLargeFont = nullptr;
std::shared_ptr<Font> Font::defaultSmallFont = nullptr;

Font::Font(std::shared_ptr<GlyphProvider> provider) : provider(provider) {}

std::shared_ptr<Font> Font::withName(const std::string& name) {
    // Check cache first
    auto it = fontCache.find(name);
    if (it != fontCache.end()) {
        return it->second;
    }

    // Try to load the font
    auto provider = loadFontFile(name);
    if (provider && provider->isValid()) {
        auto font = std::shared_ptr<Font>(new Font(provider));
        fontCache[name] = font;
        return font;
    }

    // Fallback to system font
    return systemFont();
}

std::shared_ptr<Font> Font::withProvider(
    std::shared_ptr<GlyphProvider> provider,
    const std::string& cacheKey)
{
    if (!provider || !provider->isValid()) {
        return nullptr;
    }

    // Check cache if key provided
    if (!cacheKey.empty()) {
        auto it = fontCache.find(cacheKey);
        if (it != fontCache.end()) {
            return it->second;
        }
    }

    auto font = std::shared_ptr<Font>(new Font(provider));

    // Cache if key provided
    if (!cacheKey.empty()) {
        fontCache[cacheKey] = font;
    }

    return font;
}

std::shared_ptr<Font> Font::systemFont() {
    return defaultSystemFont;
}

void Font::setFontSearchPath(const std::string& path) {
    clearSearchPaths();
    addFontSearchPath(path);
}

void Font::addFontSearchPath(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p.back() != '/') {
        p += '/';
    }
    searchPaths.push_back(p);
}

void Font::clearSearchPaths() {
    searchPaths.clear();
}

const std::vector<std::string>& Font::getSearchPaths() {
    return searchPaths;
}

void Font::setSystemFont(std::shared_ptr<Font> font) {
    defaultSystemFont = font;
}

std::shared_ptr<Font> Font::systemLargeFont() {
    return defaultLargeFont ? defaultLargeFont : systemFont();
}

void Font::setSystemLargeFont(std::shared_ptr<Font> font) {
    defaultLargeFont = font;
}

std::shared_ptr<Font> Font::systemSmallFont() {
    return defaultSmallFont ? defaultSmallFont : systemFont();
}

void Font::setSystemSmallFont(std::shared_ptr<Font> font) {
    defaultSmallFont = font;
}

void Font::clearCache() {
    fontCache.clear();
}

std::shared_ptr<GlyphProvider> Font::loadFontFile(const std::string& name) {
    for (const auto& path : searchPaths) {
        std::string baseName = name;
        // Strip any existing extension
        auto bdfPos = baseName.find(".bdf");
        if (bdfPos != std::string::npos)
            baseName = baseName.substr(0, bdfPos);
        auto bdpPos = baseName.find(".bdp");
        if (bdpPos != std::string::npos)
            baseName = baseName.substr(0, bdpPos);

        // Try .bdp first
        std::string bdpPath = path + baseName + ".bdp";
        auto packed = std::make_shared<PackedFontGlyphProvider>(bdpPath);
        if (packed->isValid()) return packed;

        // Fall back to .bdf
        std::string bdfPath = path + baseName + ".bdf";
        auto bdf = std::make_shared<BDFGlyphProvider>(bdfPath);
        if (bdf->isValid()) return bdf;
    }
    return nullptr;
}

std::string Font::getTitle() const {
    return provider ? provider->getTitle() : "";
}

uint8_t Font::getGlyphRowCount() const {
    return provider ? provider->getGlyphRowCount() : 16;
}

uint8_t Font::getPointSize() const {
    return provider ? provider->getPointSize() : 12;
}

Rect Font::metricsForCodepoint(UNICODE_CODEPOINT codepoint) {
    return provider ? provider->metricsForCodepoint(codepoint) : RectZero;
}

uint8_t* Font::glyphForCodepoint(UNICODE_CODEPOINT codepoint) {
    return provider ? provider->glyphForCodepoint(codepoint) : nullptr;
}

bool Font::isValid() const {
    return provider && provider->isValid();
}
