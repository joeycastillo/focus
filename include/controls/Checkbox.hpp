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
 * @file Checkbox.hpp
 * @brief A toggle checkbox control with a text label.
 *
 * Renders a small square indicator followed by a text label. When checked,
 * the indicator is filled; when unchecked, it is empty. Tapping or selecting
 * toggles the state and fires a FOCUS_EVENT_VALUE_CHANGED action.
 */

#pragma once

#include "Control.hpp"
#include <memory>

class Font;
class CanvasView;

/**
 * @brief A checkbox control that toggles between checked and unchecked states.
 *
 * The checkbox inverts its appearance when focused (swaps fg/bg colors).
 * Register a FOCUS_EVENT_VALUE_CHANGED action to respond to state changes.
 */
class Checkbox : public Control {
public:
    /**
     * @brief Construct a checkbox with a text label.
     * @param rect Frame rectangle.
     * @param text The label displayed next to the checkbox indicator.
     */
    Checkbox(Rect rect, std::string text);
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
    /// @brief Handle touch/select events to toggle the checked state.
    bool handleEvent(Event event) override;
    void didBecomeFocused() override;
    void didResignFocus() override;

    /// @brief Check whether the checkbox is currently checked.
    bool isChecked() const;
    /// @brief Set the checked state programmatically.
    void setChecked(bool value);
    /// @brief Set the label text.
    void setText(std::string text);
    /// @brief Set the font for the label. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the current font.
    std::shared_ptr<Font> getFont() const;

protected:
    std::string text;              ///< Label text.
    bool checked = false;          ///< Whether the checkbox is checked.
    std::shared_ptr<Font> font;    ///< Custom font, or nullptr for system font.

private:
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    void renderCanvas();
};
