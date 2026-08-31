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
 * text, font, text scale, alignment, canvas rotation, frame, or truncation
 * mode changes. Text that overflows the frame clips by
 * default; setTruncationMode(TruncationMode::Tail) ends the last fitting
 * line with an ellipsis instead. Truncation is presentation only — getText()
 * and accessibilityLabel() always return the full string.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include <memory>

namespace focus {

class Font;
class CanvasView;

/**
 * @brief A view that displays word-wrapped text.
 *
 * Uses an internal CanvasView for off-screen text rendering. If no font is
 * set, falls back to Font::systemFont().
 * @ingroup views
 */
class LabelView : public View {
public:
    /**
     * @brief Construct a label view with initial text.
     * @param rect Frame rectangle.
     * @param text The UTF-8 text to display.
     */
    LabelView(Rect rect, std::string text);
    void setFrame(Rect rect) override;
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
    void appearanceDidChange() override;

    /// @brief Set the displayed text, invalidating the cached rendering.
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
    std::shared_ptr<Font> getFont() const;
    /// @brief Set the horizontal text alignment.
    void setTextAlignment(TextAlignment alignment);
    /// @brief Get the horizontal text alignment.
    TextAlignment getTextAlignment() const { return this->textAlignment; }
    /// @brief Set the canvas rotation for this label's text rendering.
    /// Rotates the text content within the view's frame (0, 90, 180, or 270).
    void setCanvasRotation(int degrees);
    /// @brief Get the canvas rotation in degrees (0, 90, 180, or 270).
    int getCanvasRotation() const { return this->canvasRotation; }
    /// @brief Set what happens to text that overflows the frame.
    /// TruncationMode::Tail ends the last fitting line with an ellipsis;
    /// the stored text is never modified, only what is drawn.
    void setTruncationMode(TruncationMode mode);
    /// @brief Get the truncation mode.
    TruncationMode getTruncationMode() const { return this->truncationMode; }

    /// @brief Returns the displayed text.
    std::string accessibilityLabel() const override;
    /// @brief Returns AccessibilityRole::StaticText.
    AccessibilityRole accessibilityRole() const override;
    /// @brief Labels are meaningful accessibility elements.
    bool isAccessibilityElement() const override;

protected:
    std::string text;              ///< The UTF-8 text content.
    uint8_t textScale = 1;         ///< Text rendering scale factor.
    std::shared_ptr<Font> font;    ///< Custom font, or nullptr for system font.
    TextAlignment textAlignment = TextAlignment::Left; ///< Text alignment.
    TruncationMode truncationMode = TruncationMode::None; ///< Overflow handling.
    int canvasRotation = 0;        ///< Canvas rotation in degrees (0/90/180/270).
    std::shared_ptr<CanvasView> canvas; ///< Internal canvas for rendered text.
    bool canvasValid = false;           ///< Whether the canvas needs re-rendering.
    /// @brief Render the text to the internal canvas.
    /// Override to customize text rendering while reusing the canvas infrastructure.
    virtual void renderCanvas();
};

}  // namespace focus
