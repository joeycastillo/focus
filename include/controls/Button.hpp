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
 * @file Button.hpp
 * @brief A tappable button control with a text label.
 *
 * Renders a bordered rectangle with centered text. The appearance inverts
 * (swaps foreground/background) when the button has focus. Register a
 * FOCUS_EVENT_TOUCH_DOWN or FOCUS_EVENT_SELECT action to respond to presses.
 */

#pragma once

#include "Control.hpp"
#include <memory>

class Font;
class CanvasView;

/**
 * @brief A button control that displays text and responds to tap/select events.
 *
 * Uses an internal CanvasView for rendering. The button border and text are
 * drawn in the foreground color, with colors inverted when focused.
 */
class Button : public Control {
public:
    /**
     * @brief Construct a button with a text label.
     * @param rect Frame rectangle.
     * @param text The button's label text.
     */
    Button(Rect rect, std::string text);
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
    void appearanceDidChange() override;

    /// @brief Set the button's label text.
    void setText(const std::string& text);
    /// @brief Get the button's label text.
    std::string getText() const { return this->text; }
    /// @brief Set the font for the button label. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the current font.
    std::shared_ptr<Font> getFont() const;

    /// @brief Invalidate the canvas when focus is gained (inverts colors).
    void didBecomeFocused() override;
    /// @brief Invalidate the canvas when focus is lost (restores colors).
    void didResignFocus() override;

    /// @brief Returns the button's label text.
    std::string accessibilityLabel() const override;
    /// @brief Returns AccessibilityRole::Button.
    AccessibilityRole accessibilityRole() const override;
protected:
    std::string text;              ///< The button's label text.
    std::shared_ptr<Font> font;    ///< Custom font, or nullptr for system font.
    std::shared_ptr<CanvasView> canvas; ///< Internal canvas for rendering.
    bool canvasValid = false;           ///< Whether the canvas needs re-rendering.
    /// @brief Render the button text and border to the internal canvas.
    /// Override to customize button rendering while reusing the canvas infrastructure.
    virtual void renderCanvas();
};
