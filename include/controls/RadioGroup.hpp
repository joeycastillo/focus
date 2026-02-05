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
 * @file RadioGroup.hpp
 * @brief Manages mutual exclusion for a group of RadioButton controls.
 *
 * RadioGroup ensures that at most one RadioButton in the group is selected
 * at any time. When a button is selected, all other buttons in the group
 * are automatically deselected.
 */

#pragma once

#include <memory>
#include <vector>
#include <functional>

class RadioButton;

/**
 * @brief Coordinates mutual exclusion among a set of RadioButton controls.
 *
 * Add RadioButtons via addButton(). When any button in the group is selected,
 * all others are deselected and the selectionChangedCallback is invoked with
 * the index of the newly selected button.
 */
class RadioGroup {
public:
    /// @brief Add a radio button to this group.
    void addButton(std::shared_ptr<RadioButton> button);
    /// @brief Remove a radio button from this group.
    void removeButton(std::shared_ptr<RadioButton> button);
    /// @brief Get the index of the currently selected button, or -1 if none.
    int getSelectedIndex() const;
    /// @brief Get the currently selected button, or nullptr if none.
    std::shared_ptr<RadioButton> getSelectedButton() const;
    /**
     * @brief Set a callback invoked when the selection changes.
     * @param callback Function called with the index of the newly selected button.
     */
    void setSelectionChangedCallback(std::function<void(int)> callback);

    /// @brief Internal: called by RadioButton when it becomes selected. Do not call directly.
    void _buttonSelected(RadioButton* selected);

private:
    std::vector<std::weak_ptr<RadioButton>> buttons; ///< Buttons in this group.
    std::function<void(int)> selectionChangedCallback; ///< Selection change callback.
};
