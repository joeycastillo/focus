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
 * @file Control.hpp
 * @brief Base class for interactive UI controls.
 *
 * Control extends View with an enabled/disabled state and the ability to
 * receive focus. All interactive elements (Button, Checkbox, Slider, TextField,
 * etc.) inherit from Control.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"

namespace focus {

/**
 * @brief Control states for state-keyed content.
 *
 * Used with Button::setTitle() and Button::setImage() to register
 * different content for different control states. Content resolution
 * falls back to Normal when no content is registered for the current state.
 */
enum class ControlState : uint8_t {
    Normal   = 0,
    Selected = 1,
    Disabled = 2,
};

/**
 * @brief Base class for focusable, interactive UI controls.
 *
 * Controls can be enabled or disabled. When enabled, they participate in
 * focus navigation and can handle user input events.
 */
class Control : public View {
public:
    /// @brief Construct a control with the given frame rectangle.
    Control(Rect rect);
    /// @brief Check whether this control is enabled.
    bool isEnabled();
    /// @brief Enable or disable this control.
    void setEnabled(bool value);
    /// @brief Check whether this control is selected.
    bool isSelected() const;
    /// @brief Set the selected state of this control.
    virtual void setSelected(bool value);
    /// @brief Controls can become focused when enabled (returns true).
    bool canBecomeFocused() override;
    /// @brief Disabled controls ignore all events.
    bool handleEvent(Event event) override;
    /// @brief Controls are meaningful accessibility elements by default.
    bool isAccessibilityElement() const override;
protected:
    bool enabled = true; ///< Whether this control accepts input.
    bool selected = false; ///< Whether this control is in the selected state.
};

}  // namespace focus

