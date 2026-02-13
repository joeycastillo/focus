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
 * @file KeyboardView.hpp
 * @brief An on-screen keyboard for text input on touch-enabled devices.
 *
 * KeyboardView renders a QWERTY keyboard layout and handles touch events to
 * detect key presses. It supports three pages: lowercase, uppercase, and
 * symbols. Key presses are reported via a callback, with special key values
 * for backspace ("⌫"), shift ("⇧"), symbols ("123"/"ABC"), and done ("✓").
 *
 * Typically presented modally in response to a TextField being tapped.
 */

#pragma once

#include "View.hpp"
#include <string>
#include <functional>

class Font;
class CanvasView;

/**
 * @brief An on-screen touch keyboard with lowercase, uppercase, and symbol pages.
 */
class KeyboardView : public View {
public:
    /// @brief Callback type invoked when a key is pressed.
    using KeyCallback = std::function<void(std::string key)>;

    /// @brief Construct a keyboard view with the given frame and layout type.
    KeyboardView(Rect rect, KeyboardType type = KeyboardTypeDefault);

    /// @brief Set the callback invoked for each key press.
    void setKeyCallback(KeyCallback callback);
    /// @brief Set the font for key labels. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);

    void drawContent(int x, int y) override;
    /// @brief Handle touch events to detect key presses.
    bool handleEvent(Event event) override;

private:
    /// @brief Available keyboard pages.
    enum class KeyboardPage {
        Lowercase, ///< Lowercase letters.
        Uppercase, ///< Uppercase letters.
        Symbols    ///< Numbers and symbols.
    };

    /// @brief A key's hit-test rectangle and associated text.
    struct KeyRect {
        Rect rect;          ///< Hit-test rectangle in canvas coordinates.
        std::string label;  ///< Text drawn on the key cap.
        std::string value;  ///< Value sent via callback when pressed.
    };

    KeyboardType type;
    KeyCallback keyCallback;
    std::shared_ptr<Font> font;
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    KeyboardPage currentPage = KeyboardPage::Lowercase;

    void renderCanvas();
    void buildKeyLayout(std::vector<KeyRect>& keys) const;
    int getKeyForTouch(Point localPoint) const;

    mutable std::vector<KeyRect> cachedKeys;
    mutable bool keysCached = false;
};
