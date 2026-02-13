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
 * @file Slider.hpp
 * @brief A horizontal slider control for selecting a value within a range.
 *
 * Slider renders a text label on the left and a horizontal track bar on the
 * right. The filled portion of the track represents the current value (0.0 to
 * 1.0). Touch events on the track area set the value proportionally.
 *
 * Fires FOCUS_EVENT_VALUE_CHANGED with userInfo set to (int32_t)(value * 8191)
 * when the value changes, making it directly compatible with 13-bit PWM duty
 * cycle values used by the frontlight controller.
 */

#pragma once

#include "Control.hpp"
#include <memory>
#include <string>

class Font;
class CanvasView;

/**
 * @brief A horizontal slider control for continuous value selection.
 *
 * The slider displays a label on the left and an interactive track bar on
 * the right. Register a FOCUS_EVENT_VALUE_CHANGED action to respond to
 * value changes. Colors invert when focused.
 */
class Slider : public Control {
public:
    /**
     * @brief Construct a slider with a text label.
     * @param rect Frame rectangle.
     * @param label The label displayed to the left of the track bar.
     */
    Slider(Rect rect, std::string label);
    void drawContent(int x, int y) override;
    /// @brief Handle touch events to set the value from the touch position.
    bool handleEvent(Event event) override;
    void didBecomeFocused() override;
    void didResignFocus() override;

    /// @brief Get the current value (0.0 to 1.0).
    float getValue() const;
    /**
     * @brief Set the value, clamped to 0.0–1.0.
     * @param value The new value.
     */
    void setValue(float value);
    /// @brief Set the label text.
    void setLabel(std::string label);
    /// @brief Set the font for the label. Pass nullptr for system font.
    void setFont(std::shared_ptr<Font> font);
    /// @brief Get the current font.
    std::shared_ptr<Font> getFont() const;

protected:
    std::string label;             ///< Label text displayed left of the track.
    float value = 0.0f;           ///< Current value (0.0 to 1.0).
    std::shared_ptr<Font> font;    ///< Custom font, or nullptr for system font.

private:
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    void renderCanvas();
    /// @brief Calculate the horizontal start of the track area.
    int getTrackX() const;
    /// @brief Calculate the width of the track area.
    int getTrackWidth() const;
};
