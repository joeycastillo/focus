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
#include "StyledGlyphProvider.hpp"
#include "FallbackGlyphProvider.hpp"
#include "BasicGlyphProvider.hpp"
#include "focus_config.h"

namespace focus {

// Static member initialization
std::map<std::string, std::shared_ptr<Font>> Font::fontCache;
std::vector<std::string> Font::searchPaths;
std::shared_ptr<Font> Font::defaultSystemFont = nullptr;
std::shared_ptr<Font> Font::defaultLargeFont = nullptr;
std::shared_ptr<Font> Font::defaultSmallFont = nullptr;
std::function<Font::FallbackFont(const GlyphProvider*)> Font::fallbackResolver;

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

std::shared_ptr<Font> Font::familyWithName(const std::string& baseName) {
    // Check cache first
    std::string cacheKey = "family:" + baseName;
    auto it = fontCache.find(cacheKey);
    if (it != fontCache.end()) return it->second;

    // Load the regular font (required)
    auto regular = loadFontFile(baseName);
    if (!regular || !regular->isValid()) {
        // No font file by this name. It may be a provider registered at runtime
        // via withProvider (e.g. "unifont32"/"unifont48", used as per-book font
        // overrides for non-Latin scripts). Resolve those directly so the
        // override renders in the intended font instead of silently degrading to
        // the system font. Registered providers are cached under their plain name.
        auto registered = fontCache.find(baseName);
        if (registered != fontCache.end()) return registered->second;
        return systemFont();
    }

    // Try loading variants
    auto bold = loadFontFile(baseName + "-bold");
    auto italic = loadFontFile(baseName + "-italic");
    auto boldItalic = loadFontFile(baseName + "-bolditalic");

    // If no variants found, return a plain font
    bool hasVariants = (bold && bold->isValid()) ||
                       (italic && italic->isValid()) ||
                       (boldItalic && boldItalic->isValid());

    std::shared_ptr<GlyphProvider> provider;
    if (hasVariants) {
        provider = std::make_shared<StyledGlyphProvider>(
            regular,
            (italic && italic->isValid()) ? italic : nullptr,
            (bold && bold->isValid()) ? bold : nullptr,
            (boldItalic && boldItalic->isValid()) ? boldItalic : nullptr);
    } else {
        provider = regular;
    }

    // Apply fallback resolver if registered
    if (fallbackResolver) {
        FallbackFont fb = fallbackResolver(provider.get());
        if (fb.provider) {
            provider = std::make_shared<FallbackGlyphProvider>(provider, fb.provider, fb.ascent);
        }
    }

    auto font = std::shared_ptr<Font>(new Font(provider));
    fontCache[cacheKey] = font;
    return font;
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
    if (!defaultSystemFont) {
        // Floor, not a feature: with no font configured, fall back to the
        // built-in 5x8 ASCII provider so text is visibly wrong rather than
        // invisibly absent.
        defaultSystemFont = std::shared_ptr<Font>(new Font(std::make_shared<BasicGlyphProvider>()));
    }
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

void Font::setFallbackResolver(std::function<Font::FallbackFont(const GlyphProvider*)> resolver) {
    fallbackResolver = std::move(resolver);
}

std::shared_ptr<GlyphProvider> Font::loadFontFile(const std::string& name) {
#if FOCUS_HAS_FILESYSTEM
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
#else
    (void)name;
    return nullptr;
#endif
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

GlyphMetrics Font::metricsForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    return provider ? provider->metricsForCodepoint(codepoint, emphasis) : GlyphMetrics{};
}

const uint8_t* Font::glyphForCodepoint(UNICODE_CODEPOINT codepoint, FontStyle emphasis) const {
    return provider ? provider->glyphForCodepoint(codepoint, emphasis) : nullptr;
}

bool Font::isValid() const {
    return provider && provider->isValid();
}

}  // namespace focus
