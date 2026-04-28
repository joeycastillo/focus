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
 * @file StyledGlyphProvider.hpp
 * @brief A GlyphProvider that dispatches to per-emphasis sub-providers.
 *
 * StyledGlyphProvider wraps up to four single-font GlyphProviders — one for
 * each emphasis level (regular, italic, bold, bold+italic) — and routes
 * glyph/metric queries to the appropriate sub-provider based on the emphasis
 * parameter.
 *
 * This is the bridge between the emphasis-aware text rendering stack and the
 * emphasis-unaware single-font providers (BDFGlyphProvider, PackedFontGlyphProvider,
 * etc.). Each sub-provider handles exactly one style; StyledGlyphProvider
 * selects among them.
 *
 * When a requested emphasis level has no provider, queries fall back to the
 * regular (emphasis=0) provider. supportsEmphasis() reports which levels have
 * real font data vs. fallback.
 */

#pragma once

#include "GlyphProvider.hpp"
#include <memory>
#include <array>

/**
 * @brief GlyphProvider that dispatches to per-emphasis sub-providers.
 *
 * Emphasis values: 0=regular, 1=italic, 2=bold, 3=bold+italic.
 *
 * @par Construction
 * The regular provider is required. Bold, italic, and bold+italic providers
 * are optional — pass nullptr for unsupported styles.
 *
 * @par Fallback
 * When a requested emphasis level has no provider, all queries fall back to
 * the regular provider. Font-level metadata (point size, max size, offset,
 * row count, title, validity) always comes from the regular provider, since
 * all variants in a family share the same ascent and descent.
 *
 * @ingroup text
 */
class StyledGlyphProvider : public GlyphProvider {
public:
    /**
     * @brief Create a styled provider from up to four single-font providers.
     *
     * Parameter order matches emphasis values: 0=regular, 1=italic, 2=bold,
     * 3=bold+italic.
     *
     * @param regular The regular (emphasis=0) provider. Must not be nullptr.
     * @param italic The italic (emphasis=1) provider, or nullptr if unsupported.
     * @param bold The bold (emphasis=2) provider, or nullptr if unsupported.
     * @param boldItalic The bold+italic (emphasis=3) provider, or nullptr if unsupported.
     */
    StyledGlyphProvider(
        std::shared_ptr<GlyphProvider> regular,
        std::shared_ptr<GlyphProvider> italic = nullptr,
        std::shared_ptr<GlyphProvider> bold = nullptr,
        std::shared_ptr<GlyphProvider> boldItalic = nullptr);

    // GlyphProvider interface — font-level metadata delegates to regular provider
    uint8_t getPointSize() const override;
    Size getMaxSize() const override;
    Point getOffset() const override;
    uint8_t getGlyphRowCount() const override;
    bool isValid() const override;
    std::string getTitle() const override;

    // GlyphProvider interface — per-glyph queries dispatch on emphasis
    const uint8_t *glyphForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;
    GlyphMetrics metricsForCodepoint(UNICODE_CODEPOINT codepoint, uint8_t emphasis = 0) const override;
    bool hasGlyph(UNICODE_CODEPOINT codepoint) const override;

    /// Query whether this provider has a real font for the given emphasis level.
    /// Returns true only if a non-null provider was supplied for that level.
    bool supportsEmphasis(uint8_t emphasis) const override;

private:
    /// Get the sub-provider for the given emphasis, falling back to regular.
    const GlyphProvider* providerForEmphasis(uint8_t emphasis) const;

    /// [0]=regular, [1]=italic, [2]=bold, [3]=bold+italic
    std::array<std::shared_ptr<GlyphProvider>, 4> providers;
};
