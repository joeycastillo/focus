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

#include "Focus.hpp"
#include "GlyphProvider.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace focus {

/// Font provides a UIKit-style cached factory for loading and managing fonts.
/// Fonts are automatically cached — requesting the same font name twice returns
/// the same instance.
///
/// Usage:
///   // At app startup
///   Font::setFontSearchPath("/system/fonts/");
///   Font::setSystemFont(Font::withName("spleen-12x24"));
///
///   // In views
///   label->setFont(Font::withName("spleen-16x32"));
///
/// @par Memory management
/// The font cache grows without bound as new fonts are loaded. On
/// memory-constrained devices, call clearCache() when the platform
/// signals memory pressure (e.g., a low-heap-watermark callback, a
/// screen transition that retires a font set, or periodic maintenance).
/// clearCache() is always safe to call: fonts actively held by views
/// survive via shared_ptr, and the system fonts (systemFont(),
/// systemLargeFont(), systemSmallFont()) are stored separately and
/// are not affected.
///

class Font {
public:
    /// Return type for the fallback font resolver callback.
    /// @see Font::setFallbackResolver
    struct FallbackFont {
        std::shared_ptr<GlyphProvider> provider;  ///< The fallback glyph provider, or nullptr for no fallback.
        uint8_t ascent = 0;  ///< Number of ascent rows in the fallback font (for baseline alignment).
    };

    /// Get a font by name. Returns cached instance if already loaded.
    /// Falls back to systemFont() if the font cannot be loaded.
    /// @param name Font name (e.g., "spleen-12x24" or "spleen-12x24.bdf")
    static std::shared_ptr<Font> withName(const std::string& name);

    /// Load a font family by base name, auto-discovering style variants.
    ///
    /// Searches for font files matching the base name with suffixes:
    ///   - (none)          -> regular
    ///   - "-bold"         -> bold
    ///   - "-italic"       -> italic
    ///   - "-bolditalic"   -> bold+italic
    ///
    /// Returns a Font backed by StyledGlyphProvider if any variants are found,
    /// or a regular Font if only the base font exists. Falls back to systemFont()
    /// if the base font cannot be loaded.
    ///
    /// @param baseName Base font name (e.g., "lucida-bright-19")
    static std::shared_ptr<Font> familyWithName(const std::string& baseName);

    /// Create a font from a custom GlyphProvider.
    /// Use this for fonts that don't come from files (e.g., Unifont, embedded fonts).
    /// @param provider The glyph provider to use.
    /// @param cacheKey Optional key for caching. If empty, font is not cached.
    /// @return A shared_ptr to the font, or nullptr if provider is invalid.
    static std::shared_ptr<Font> withProvider(
        std::shared_ptr<GlyphProvider> provider,
        const std::string& cacheKey = "");

    /// Get the system/default font.
    /// @return The system font, or nullptr if not set.
    static std::shared_ptr<Font> systemFont();

    /// Get the system large font (for titles, alert headings, etc.).
    /// @return The large font, or systemFont() if not set.
    static std::shared_ptr<Font> systemLargeFont();

    /// Get the system small font (for fine print, captions, etc.).
    /// @return The small font, or systemFont() if not set.
    static std::shared_ptr<Font> systemSmallFont();

    /// Set the path where font files are located (clears any existing paths).
    /// @param path Directory path (e.g., "/system/fonts/")
    static void setFontSearchPath(const std::string& path);

    /// Add an additional search path for font files.
    /// Paths are searched in the order they are added.
    /// @param path Directory path (e.g., "/fonts/")
    static void addFontSearchPath(const std::string& path);

    /// Clear all font search paths.
    static void clearSearchPaths();

    /// Get the current list of search paths.
    static const std::vector<std::string>& getSearchPaths();

    /// Set the system/default font.
    /// @param font The font to use as the system default.
    static void setSystemFont(std::shared_ptr<Font> font);

    /// Set the system large font (for titles, alert headings, etc.).
    /// @param font The font to use as the large font.
    static void setSystemLargeFont(std::shared_ptr<Font> font);

    /// Set the system small font (for fine print, captions, etc.).
    /// @param font The font to use as the small font.
    static void setSystemSmallFont(std::shared_ptr<Font> font);

    /// Clear the font cache, releasing all cached Font instances.
    ///
    /// Safe to call at any time. Fonts currently held by views (or any
    /// other shared_ptr holder) remain valid — only the cache's own
    /// reference is dropped. The next withName() call for a cleared font
    /// will reload it from disk.
    ///
    /// System fonts (set via setSystemFont / setSystemLargeFont /
    /// setSystemSmallFont) are stored separately and are not affected.
    ///
    /// Call this from your platform's memory-pressure handler, or at
    /// natural transition points (e.g., changing themes or locales)
    /// where the active font set changes.
    static void clearCache();

    /// Register an application-defined callback for automatic font fallback.
    ///
    /// When set, familyWithName() calls the resolver with the primary provider.
    /// If the resolver returns a non-null provider, the result is automatically
    /// wrapped with FallbackGlyphProvider. This enables per-glyph fallback to
    /// a secondary font (e.g., Unifont) for codepoints the primary lacks.
    ///
    /// @note The resolver must be set before any familyWithName() calls.
    /// familyWithName() caches its results, and changing the resolver does not
    /// invalidate already-cached fonts. Fonts loaded before the resolver was
    /// set will not have fallback wrapping applied.
    ///
    /// Pass nullptr to clear the resolver and disable automatic fallback.
    ///
    /// @param resolver A function that maps a primary provider to a fallback
    ///        font, or nullptr to disable.
    static void setFallbackResolver(std::function<FallbackFont(const GlyphProvider*)> resolver);

    /// Access the underlying GlyphProvider (for compatibility with existing code).
    GlyphProvider* getGlyphProvider() const { return provider.get(); }

    /// Access the underlying GlyphProvider as a shared_ptr.
    std::shared_ptr<GlyphProvider> getSharedGlyphProvider() const { return provider; }

    // Convenience methods delegating to GlyphProvider
    std::string getTitle() const;
    uint8_t getGlyphRowCount() const;
    uint8_t getPointSize() const;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const;
    const uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const;
    bool isValid() const;

private:
    // Private constructor - use factory methods
    Font(std::shared_ptr<GlyphProvider> provider);

    std::shared_ptr<GlyphProvider> provider;

    // Static members for caching
    static std::map<std::string, std::shared_ptr<Font>> fontCache;
    static std::vector<std::string> searchPaths;
    static std::shared_ptr<Font> defaultSystemFont;
    static std::shared_ptr<Font> defaultLargeFont;
    static std::shared_ptr<Font> defaultSmallFont;
    static std::function<FallbackFont(const GlyphProvider*)> fallbackResolver;

    // Helper to load a font file
    static std::shared_ptr<GlyphProvider> loadFontFile(const std::string& name);
};

}  // namespace focus
