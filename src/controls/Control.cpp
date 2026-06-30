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

#include "Control.hpp"
#include "Window.hpp"

namespace focus {

Control::Control(Rect rect) : View(rect) {
}

bool Control::isEnabled() {
    return this->enabled;
}

void Control::setEnabled(bool value) {
    if (this->enabled != value) {
        this->enabled = value;
        this->appearanceDidChange();
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
        }
    }
}

bool Control::isSelected() const {
    return this->selected;
}

void Control::setSelected(bool value) {
    if (this->selected != value) {
        this->selected = value;
        this->appearanceDidChange();
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
        }
    }
}

bool Control::canBecomeFocused() {
    return this->enabled;
}

bool Control::handleEvent(Event event) {
    if (!this->enabled) return false;
    return View::handleEvent(event);
}

bool Control::isAccessibilityElement() const {
    return true;
}

}  // namespace focus
