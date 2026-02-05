/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
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
 * @file LabelView.hpp
 * @brief View that renders a text string with word wrapping.
 *
 * LabelView displays a UTF-8 text string rendered with a configurable font
 * and text scale. The text is word-wrapped within the view's frame and rendered
 * to an internal CanvasView, which is cached and re-rendered only when the
 * text, font, or frame changes.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <memory>

class Font;
class CanvasView;

/**
 * @brief A view that displays word-wrapped text.
 *
 * Uses an internal CanvasView for off-screen text rendering. If no font is
 * set, falls back to Font::systemFont().
 */
class LabelView : public View {
public:
    /**
     * @brief Construct a label view with initial text.
     * @param rect Frame rectangle.
     * @param text The UTF-8 text to display.
     */
    LabelView(Rect rect, std::string text);
    void draw(int x, int y) override;

    /// @brief Set the displayed text, invalidating the cached rendering.
    void setText(std::string text);
    /// @brief Set the text scale factor (1 = normal, 2 = double size, etc.).
    void setTextScale(uint8_t scale);
    /// @brief Set the font to use. Pass nullptr to use the system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the currently assigned font (may be nullptr for system font).
    std::shared_ptr<Font> getFont() const;
    /// @brief Set the horizontal text alignment.
    void setTextAlignment(TextAlignment alignment);
protected:
    std::string text;              ///< The UTF-8 text content.
    uint8_t textScale = 1;         ///< Text rendering scale factor.
    std::shared_ptr<Font> font;    ///< Custom font, or nullptr for system font.
    TextAlignment textAlignment = TextAlignmentLeft; ///< Text alignment.
private:
    std::shared_ptr<CanvasView> canvas; ///< Internal canvas for rendered text.
    bool canvasValid = false;           ///< Whether the canvas needs re-rendering.
    void renderCanvas();                ///< Render the text to the internal canvas.
};
