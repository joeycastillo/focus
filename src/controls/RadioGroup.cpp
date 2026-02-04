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

#include "RadioGroup.hpp"
#include "RadioButton.hpp"

void RadioGroup::addButton(std::shared_ptr<RadioButton> button) {
    this->buttons.push_back(button);
}

void RadioGroup::removeButton(std::shared_ptr<RadioButton> button) {
    this->buttons.erase(
        std::remove_if(this->buttons.begin(), this->buttons.end(),
            [&button](const std::weak_ptr<RadioButton>& wp) {
                auto sp = wp.lock();
                return !sp || sp == button;
            }),
        this->buttons.end());
}

int RadioGroup::getSelectedIndex() const {
    int index = 0;
    for (const auto& wp : this->buttons) {
        if (auto sp = wp.lock()) {
            if (sp->isSelected()) return index;
        }
        index++;
    }
    return -1;
}

std::shared_ptr<RadioButton> RadioGroup::getSelectedButton() const {
    for (const auto& wp : this->buttons) {
        if (auto sp = wp.lock()) {
            if (sp->isSelected()) return sp;
        }
    }
    return nullptr;
}

void RadioGroup::setSelectionChangedCallback(std::function<void(int)> callback) {
    this->selectionChangedCallback = callback;
}

void RadioGroup::_buttonSelected(RadioButton* selected) {
    int selectedIndex = -1;
    int index = 0;
    for (const auto& wp : this->buttons) {
        if (auto sp = wp.lock()) {
            if (sp.get() == selected) {
                selectedIndex = index;
            } else {
                sp->setSelected(false);
            }
        }
        index++;
    }
    if (this->selectionChangedCallback && selectedIndex >= 0) {
        this->selectionChangedCallback(selectedIndex);
    }
}
