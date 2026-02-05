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

/**
 * @file TextField.hpp
 * @brief An editable text input control.
 *
 * TextField displays editable text within a bordered rectangle. When tapped,
 * it presents a KeyboardView for text entry. Supports placeholder text, max
 * length limits, and a text-changed callback.
 */

#pragma once

#include "Control.hpp"
#include <string>
#include <functional>

class Font;
class CanvasView;

/**
 * @brief An editable single-line text input control.
 *
 * Renders the current text (or placeholder when empty) in a bordered box.
 * When touched, presents an on-screen keyboard for editing. Subclass
 * getDisplayText() to customize rendering (e.g. PasswordField masks characters).
 */
class TextField : public Control {
public:
    /// @brief Construct an empty text field with the given frame.
    TextField(Rect rect);

    /// @brief Get the current text content.
    std::string getText() const;
    /// @brief Set the text content.
    void setText(std::string text);
    /// @brief Set placeholder text shown when the field is empty.
    void setPlaceholder(std::string placeholder);
    /**
     * @brief Set the maximum number of characters allowed.
     * @param maxLength Maximum length, or 0 for no limit.
     */
    void setMaxLength(size_t maxLength);
    /// @brief Set the font. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the current font.
    std::shared_ptr<Font> getFont() const;
    /**
     * @brief Set a callback invoked whenever the text changes.
     * @param callback Function called with the new text value.
     */
    void setTextChangedCallback(std::function<void(std::string)> callback);

    void draw(int x, int y) override;
    /// @brief Handle touch events to request focus (and thereby the keyboard).
    bool handleEvent(Event event) override;
    void didBecomeFocused() override;
    void didResignFocus() override;

    bool wantsKeyboardInput() override;
    void insertText(const std::string& str) override;
    void deleteBackward() override;

protected:
    /**
     * @brief Get the text to render on screen.
     *
     * Override in subclasses for custom display (e.g. masking for passwords).
     * The default implementation returns the actual text content.
     */
    virtual std::string getDisplayText() const;

    std::string text;                    ///< Current text content.
    std::string placeholder;             ///< Placeholder shown when text is empty.
    size_t maxLength = 0;                ///< Max character count (0 = unlimited).
    std::shared_ptr<Font> font;          ///< Custom font, or nullptr for system font.
    std::function<void(std::string)> textChangedCallback; ///< Text change callback.

private:
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    void renderCanvas();
};
