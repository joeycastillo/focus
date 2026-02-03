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

#pragma once

#include "Focus.hpp"
#include "GlyphProvider.hpp"
#include <string>
#include <map>
#include <memory>

/// Font provides a UIKit-style cached factory for loading and managing fonts.
/// Fonts are automatically cached - requesting the same font name twice returns
/// the same instance.
///
/// Usage:
///   // At app startup
///   Font::setFontSearchPath("/sdcard/fonts/");
///   Font::setSystemFont(Font::withName("timR18"));
///
///   // In views
///   label->setFont(Font::withName("timR24"));
///
class Font {
public:
    /// Get a font by name. Returns cached instance if already loaded.
    /// Falls back to systemFont() if the font cannot be loaded.
    /// @param name Font name (e.g., "timR18" or "timR18.bdf")
    static std::shared_ptr<Font> withName(const std::string& name);

    /// Get the system/default font.
    /// @return The system font, or nullptr if not set.
    static std::shared_ptr<Font> systemFont();

    /// Set the path where font files are located.
    /// @param path Directory path (e.g., "/sdcard/fonts/")
    static void setFontSearchPath(const std::string& path);

    /// Set the system/default font.
    /// @param font The font to use as the system default.
    static void setSystemFont(std::shared_ptr<Font> font);

    /// Clear the font cache. Useful for testing or memory pressure.
    static void clearCache();

    /// Access the underlying GlyphProvider (for compatibility with existing code).
    GlyphProvider* getGlyphProvider() const { return provider.get(); }

    /// Access the underlying GlyphProvider as a shared_ptr.
    std::shared_ptr<GlyphProvider> getSharedGlyphProvider() const { return provider; }

    // Convenience methods delegating to GlyphProvider
    uint8_t getGlyphRowCount() const;
    uint8_t getPointSize() const;
    Rect metricsForCodepoint(UNICODE_CODEPOINT codepoint);
    uint8_t* glyphForCodepoint(UNICODE_CODEPOINT codepoint);
    bool isValid() const;

private:
    // Private constructor - use factory methods
    Font(std::shared_ptr<GlyphProvider> provider);

    std::shared_ptr<GlyphProvider> provider;

    // Static members for caching
    static std::map<std::string, std::shared_ptr<Font>> fontCache;
    static std::string searchPath;
    static std::shared_ptr<Font> defaultSystemFont;

    // Helper to load a font file
    static std::shared_ptr<GlyphProvider> loadFontFile(const std::string& name);
};
