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
 * @file RadioButton.hpp
 * @brief A mutually exclusive radio button control.
 *
 * RadioButton renders a circular indicator followed by a text label. When
 * selected, the indicator is filled. RadioButtons should be grouped using a
 * RadioGroup, which ensures that only one button in the group is selected
 * at a time.
 */

#pragma once

#include "Control.hpp"
#include <memory>

class Font;
class CanvasView;
class RadioGroup;

/**
 * @brief A radio button control for mutually exclusive selection.
 *
 * Use with RadioGroup for automatic mutual exclusion. Register a
 * FOCUS_EVENT_VALUE_CHANGED action to respond to selection changes.
 */
class RadioButton : public Control {
public:
    /**
     * @brief Construct a radio button with a text label.
     * @param rect Frame rectangle.
     * @param text The label displayed next to the radio indicator.
     */
    RadioButton(Rect rect, std::string text);
    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;
    /// @brief Handle touch/select events to select this radio button.
    bool handleEvent(Event event) override;
    void didBecomeFocused() override;
    void didResignFocus() override;

    /// @brief Check whether this radio button is currently selected.
    bool isSelected() const;
    /// @brief Set the selection state programmatically.
    void setSelected(bool value);
    /// @brief Set the label text.
    void setText(std::string text);
    /// @brief Set the font for the label. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the current font.
    std::shared_ptr<Font> getFont() const;
    /**
     * @brief Associate this radio button with a RadioGroup.
     *
     * The group ensures mutual exclusion: selecting this button deselects
     * all others in the same group.
     * @param group The RadioGroup to join.
     */
    void setGroup(std::shared_ptr<RadioGroup> group);

protected:
    std::string text;                     ///< Label text.
    bool selected = false;                ///< Whether this button is selected.
    std::shared_ptr<Font> font;           ///< Custom font, or nullptr for system font.
    /// The RadioGroup this button belongs to. This is a shared_ptr (not weak)
    /// so the group stays alive as long as any button in it exists. There is no
    /// ownership cycle because RadioGroup holds only weak_ptrs back to its buttons.
    std::shared_ptr<RadioGroup> group;

private:
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    void renderCanvas();
};
