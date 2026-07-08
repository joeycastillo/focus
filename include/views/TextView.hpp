/*
 * MIT License
 *
 * Copyright (c) 2022-2026 Joey Castillo
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
 * @file TextView.hpp
 * @brief View that renders long-form word-wrapped text lazily.
 *
 * TextView keeps a small index of laid-out lines and rasterizes only the
 * lines that intersect the clip rect on each draw, so its memory use is
 * independent of the text length. Use it for long scrollable text; use
 * LabelView for short cached text.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <memory>
#include <string>
#include <vector>

namespace focus {

class Font;
class CanvasView;
class GlyphProvider;

/**
 * @brief A view that displays long-form word-wrapped text, rendered lazily.
 *
 * Unlike LabelView, which rasterizes its whole frame into a cached canvas,
 * TextView keeps a per-line index (~12 bytes per line) and a one-line
 * scratch buffer, rasterizing only clip-visible lines on each draw. It
 * holds its entire string in memory.
 *
 * Size the frame with heightForWidth() before display; inside a ScrollView,
 * pass the same size to setContentSize().
 * @ingroup views
 */
class TextView : public View {
public:
    /**
     * @brief Construct a text view with initial text.
     * @param rect Frame rectangle. The width drives word wrapping.
     * @param text The UTF-8 text to display.
     */
    TextView(Rect rect, std::string text);
    void setFrame(Rect rect) override;
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;

    /// @brief Set the displayed text, invalidating the line index.
    void setText(std::string text);
    /// @brief Get the displayed text.
    std::string getText() const { return this->text; }
    /// @brief Set the text scale factor (1 = normal, 2 = double size, etc.).
    void setTextScale(uint8_t scale);
    /// @brief Get the text scale factor.
    uint8_t getTextScale() const { return this->textScale; }
    /// @brief Set the font to use. Pass nullptr to use the system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the currently assigned font (may be nullptr for system font).
    std::shared_ptr<Font> getFont() const { return this->font; }
    /// @brief Set the horizontal text alignment.
    void setTextAlignment(TextAlignment alignment);
    /// @brief Get the horizontal text alignment.
    TextAlignment getTextAlignment() const { return this->textAlignment; }

    /**
     * @brief Measure the height needed to lay out the text at a given width.
     * @param width The layout width in pixels.
     * @return Total height in pixels, including line and paragraph spacing.
     */
    int heightForWidth(int width) const;

    /// @brief Returns the displayed text.
    std::string accessibilityLabel() const override;
    /// @brief Returns AccessibilityRole::StaticText.
    AccessibilityRole accessibilityRole() const override;
    /// @brief Text views are meaningful accessibility elements.
    bool isAccessibilityElement() const override;

private:
    /// One laid-out line: byte range, position, and the wrap decisions the
    /// renderer must reproduce.
    struct LineRecord {
        uint32_t startByte;       ///< Byte offset of the line's first byte.
        uint32_t endByte;         ///< Byte offset past the line's last byte.
        int16_t y;                ///< Top of the line in content coordinates.
        uint8_t emphasisAtStart;  ///< SO/SI emphasis depth at the line start.
        bool hasTrailingHyphen;   ///< Render a synthesized hyphen after the line.
    };

    std::string text;
    uint8_t textScale = 1;
    std::shared_ptr<Font> font;
    TextAlignment textAlignment = TextAlignment::Left;
    std::vector<LineRecord> lines;
    size_t maxLineCodepoints = 0;
    bool indexValid = false;
    std::shared_ptr<CanvasView> scratch;  ///< One-line render canvas.

    GlyphProvider* resolveGlyphProvider() const;
    void invalidateIndex();
    void rebuildIndex();
};

}  // namespace focus
