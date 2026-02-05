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

#include "Window.hpp"
#include "View.hpp"
#include "Display.hpp"
#include "KeyboardView.hpp"

Window::Window(std::shared_ptr<Display> display, Size size) : View(MakeRect(0, 0, size.width, size.height)) {
    this->display = display;
    this->setNeedsDisplay(true);
}

void Window::addSubview(std::shared_ptr<View> view) {
    view->setWindow(std::static_pointer_cast<Window>(this->shared_from_this()));
    View::addSubview(view);
    // if nothing is focused, make this new view focused
    if (!this->focusedView.lock() && view->canBecomeFocused()) {
        view->becomeFocused();
    }
}

bool Window::canBecomeFocused() {
    return true;
}

void Window::setTouchEnabled() {
    this->touchEnabled = true;
    this->becomeFocused();
}

bool Window::isTouchEnabled() {
    return this->touchEnabled;
}

bool Window::needsDisplay() {
    return this->dirty;
}

void Window::setNeedsDisplay(bool needsDisplay) {
    if (needsDisplay) {
        this->dirtyRect = MakeRect(0, 0, this->frame.size.width, this->frame.size.height);
        this->dirty = true;
    } else {
        this->dirty = false;
    }
}

void Window::setNeedsDisplayInRect(Rect rect) {
    Rect finalRect;
    if (this->dirty) {
        finalRect = MakeRect(std::min(this->dirtyRect.origin.x, rect.origin.x), std::min(this->dirtyRect.origin.y, rect.origin.y), 0, 0);
        finalRect.size.width = std::max(this->dirtyRect.origin.x + this->dirtyRect.size.width, rect.origin.x + rect.size.width) - finalRect.origin.x;
        finalRect.size.height = std::max(this->dirtyRect.origin.y + this->dirtyRect.size.height, rect.origin.y + rect.size.height) - finalRect.origin.y;
    } else {
        finalRect = rect;
    }

    this->dirty = true;
    this->dirtyRect = finalRect;
}

Rect Window::getDirtyRect() {
    if (this->dirty) return this->dirtyRect;
    else return {{0,0},{0,0}};
}

std::weak_ptr<Display>Window::getDisplay() {
    return this->display;
}

std::weak_ptr<View> Window::getFocusedView() {
    return this->focusedView;
}

std::weak_ptr<View> Window::getSuperview() {
    return std::weak_ptr<View>();
}

std::weak_ptr<Window> Window::getWindow() {
    return std::static_pointer_cast<Window, View>(this->shared_from_this());
}

void Window::setWindow(std::shared_ptr<Window> window) {
    // nothing to do here
}

std::weak_ptr<View> Window::getCapturedTouchView() {
    return this->capturedTouchView;
}

void Window::setCapturedTouchView(std::weak_ptr<View> view, Point touchDownPoint) {
    this->capturedTouchView = view;
    this->touchDownPoint = touchDownPoint;
}

void Window::clearCapturedTouchView() {
    this->capturedTouchView.reset();
    this->touchDownPoint = MakePoint(0, 0);
}

Point Window::getTouchDownPoint() {
    return this->touchDownPoint;
}

void Window::onFocusedViewChanged() {
    std::shared_ptr<View> focused = this->focusedView.lock();
    if (focused && focused->wantsKeyboardInput()) {
        presentKeyboard();
    } else {
        dismissKeyboard();
    }
}

void Window::presentKeyboard() {
    if (this->keyboard) return;

    int keyboardHeight = 260;
    this->keyboard = std::make_shared<KeyboardView>(
        MakeRect(0, this->frame.size.height - keyboardHeight,
                 this->frame.size.width, keyboardHeight));
    this->keyboard->setKeyCallback(
        std::bind(&Window::onKeyPressed, this, std::placeholders::_1));
    this->addSubview(this->keyboard);
}

void Window::dismissKeyboard() {
    if (!this->keyboard) return;

    std::shared_ptr<KeyboardView> kb = this->keyboard;
    this->keyboard.reset();
    this->removeSubview(kb);
}

void Window::onKeyPressed(std::string key) {
    std::shared_ptr<View> focused = this->focusedView.lock();
    if (!focused) return;

    if (key == "\b") {
        focused->deleteBackward();
    } else if (key == "\n") {
        this->becomeFocused();
    } else {
        focused->insertText(key);
    }
}

bool Window::isKeyboardView(std::shared_ptr<View> view) {
    if (!this->keyboard) return false;

    std::shared_ptr<View> v = view;
    while (v) {
        if (v == this->keyboard) return true;
        v = v->getSuperview().lock();
    }
    return false;
}
