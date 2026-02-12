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

#include "View.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include <algorithm>
#include <cxxabi.h>

View::View(Rect rect) {
    // printf("Creating view %p\n", this);
    this->frame = rect;
    // bounds has origin at (0,0) in the view's local coordinate system
    this->bounds = MakeRect(0, 0, rect.size.width, rect.size.height);
    this->foregroundColor = GrayscaleColor::DefaultForegroundColor();
    this->backgroundColor = GrayscaleColor::DefaultBackgroundColor();
    this->window.reset();
    this->superview.reset();
}

View::~View() {
    // printf("Destroying view %p\n", this);
}

void View::draw(int x, int y) {
    // printf("Drawing view %p\n", this);
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        if (this->opaque) {
            display->fillRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->backgroundColor);
        }
        // Subviews are positioned relative to this view's bounds origin.
        // We pass the accumulated offset so subviews draw at the correct screen position.
        int subviewX = x + this->frame.origin.x - this->bounds.origin.x;
        int subviewY = y + this->frame.origin.y - this->bounds.origin.y;
        for(std::shared_ptr<View> view : this->subviews) {
            if (!view->hidden) view->draw(subviewX, subviewY);
        }
    }
}

void View::addSubview(std::shared_ptr<View> view) {
    view->superview = this->shared_from_this();
    this->subviews.push_back(view);
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        view->setWindow(window);
        window->setNeedsDisplay(true);
    }
}

void View::removeSubview(std::shared_ptr<View> view) {
    if (view->isFocused()) {
        view->resignFocus();
    }
    view->superview.reset();
    view->window.reset();
    int index = std::distance(this->subviews.begin(), std::find(this->subviews.begin(), this->subviews.end(), view));
    this->subviews.erase(this->subviews.begin() + index);    
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        // FIXME: We should only refocus if we know the focused view was removed.
        window->becomeFocused();
        window->setNeedsDisplay(true);
    }
}

bool View::isFocused() {
    return this->focused;
}

bool View::canBecomeFocused() {
    return false;
}

bool View::becomeFocused() {
    if (this->canBecomeFocused()) {
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            std::shared_ptr<View> oldResponder = window->getFocusedView().lock();
            if (oldResponder != NULL) {
                // if the window has a focused view, let it know it's going out of focus.
                oldResponder->willResignFocus();
                oldResponder->focused = false;
                window->focusedView.reset();
                oldResponder->didResignFocus();
            }
            // then become focused ourselves.
            this->willBecomeFocused();
            this->focused = true;
            window->focusedView = this->shared_from_this();
            this->didBecomeFocused();
            window->onFocusedViewChanged();
        }

        return true;
    }

    return false;
}

void View::resignFocus() {
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        if (std::shared_ptr<View> superview = this->superview.lock()) {
            // when resigining focus (due to being removed from a superview), pass focus to the superview.
            superview->becomeFocused();
        }
    }
}

void View::movedToWindow() {
    // nothing to do here
}

void View::willBecomeFocused() {
    // nothing to do here
}

void View::didBecomeFocused() {
    if (this->superview.lock()) {
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            std::shared_ptr<View> shared_this = this->shared_from_this();
            shared_this->setNeedsDisplayInRect(this->frame);
        }
    }
}

void View::willResignFocus() {
    // nothing to do here
}

void View::didResignFocus() {
    if (this->superview.lock()) {
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            std::shared_ptr<View> shared_this = this->shared_from_this();
            shared_this->setNeedsDisplayInRect(this->frame);
        }
    }
}

bool View::handleEvent(Event event) {
    std::shared_ptr<View> focusedView = NULL;
    std::shared_ptr<Window> window = NULL;
    if ((window = this->getWindow().lock())) {
        focusedView = window->getFocusedView().lock();
    } else {
        focusedView = this->shared_from_this();
        if (focusedView == NULL) return false;
        window = std::static_pointer_cast<Window, View>(focusedView);
    }

    if (this->actions.count(event.type)) {
        // if an action has registered for this type of event, pass it along directly
        if (std::shared_ptr<Application> application = window->application.lock()) {
            this->actions[event.type](event, this->shared_from_this());
        }
    } else {
        // otherwise, some events are handled internally
        switch (event.type) {
            case FOCUS_EVENT_DIRECTION_LEFT:
            case FOCUS_EVENT_DIRECTION_DOWN:
            case FOCUS_EVENT_DIRECTION_UP:
            case FOCUS_EVENT_DIRECTION_RIGHT:
            {
                uint32_t index = std::distance(this->subviews.begin(), std::find(this->subviews.begin(), this->subviews.end(), focusedView));
                if (this->affinity == DirectionalAffinityVertical) {
                    switch (event.type) {
                        case FOCUS_EVENT_DIRECTION_UP:
                            while (index > 0) {
                                if (this->subviews[index - 1]->canBecomeFocused()) this->subviews[index - 1]->becomeFocused();
                                else index--;
                                return true;
                            }
                            break;
                        case FOCUS_EVENT_DIRECTION_DOWN:
                            while ((index + 1) < this->subviews.size()) {
                                if (this->subviews[index + 1]->canBecomeFocused()) this->subviews[index + 1]->becomeFocused();
                                else index--;
                                return true;
                            }
                            break;
                        default:
                            break;
                    }
                } else if (this->affinity == DirectionalAffinityHorizontal) {
                    switch (event.type) {
                        case FOCUS_EVENT_DIRECTION_LEFT:
                            while (index > 0) {
                                if (this->subviews[index - 1]->canBecomeFocused()) this->subviews[index - 1]->becomeFocused();
                                return true;
                            }
                            break;
                        case FOCUS_EVENT_DIRECTION_RIGHT:
                            while ((index + 1) < this->subviews.size()) {
                                if (this->subviews[index + 1]->canBecomeFocused()) this->subviews[index + 1]->becomeFocused();
                                return true;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }
    }

    if (std::shared_ptr<View> superview = this->superview.lock()) {
        // if the event was not handled internally, bubble it up to the next view in the hierarchy.
        superview->handleEvent(event);
    }

    return false;
}

void View::setAction(const Action &action, int32_t type) {
    this->actions[type] = action;
}

void View::removeAction(int32_t type) {
    // TODO: remove the action
}

std::weak_ptr<View> View::getSuperview() {
    return this->superview;
}

std::weak_ptr<Window> View::getWindow() {
    return this->window;
}

void View::setWindow(std::shared_ptr<Window>window) {
    this->window = window;
    for(std::shared_ptr<View> subview : this->subviews) {
        subview->setWindow(window);
    }
}

std::shared_ptr<Display> View::getDisplayIfAttached() {
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        return window->getDisplay().lock();
    }
    return nullptr;
}

Rect View::getFrame() {
    return this->frame;
}

void View::setFrame(Rect frame) {
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        Rect dirtyRect = MakeRect(std::min(this->frame.origin.x, frame.origin.x), std::min(this->frame.origin.y, frame.origin.y), 0, 0);
        dirtyRect.size.width = std::max(this->frame.origin.x + this->frame.size.width, frame.origin.x + frame.size.width) - dirtyRect.origin.x;
        dirtyRect.size.height = std::max(this->frame.origin.y + this->frame.size.height, frame.origin.y + frame.size.height) - dirtyRect.origin.y;
        this->frame = frame;
        // Keep bounds size in sync with frame size
        this->bounds.size = frame.size;
        this->setNeedsDisplayInRect(dirtyRect);
    } else {
        this->frame = frame;
        this->bounds.size = frame.size;
    }
}

Rect View::getBounds() {
    return this->bounds;
}

void View::setBounds(Rect bounds) {
    this->bounds = bounds;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

bool View::isOpaque() {
    return this->opaque;
}

void View::setOpaque(bool value) {
    if (this-> opaque == value) return;

    this->opaque = value;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

bool View::isHidden() {
    return this->hidden;
}

void View::setHidden(bool value) {
    if (this-> hidden == value) return;

    this->hidden = value;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

uint16_t View::getBackgroundColor() {
    return this->backgroundColor;
}

void View::setBackgroundColor(uint16_t value) {
    this->backgroundColor = value;
}

uint16_t View::getForegroundColor() {
    return this->foregroundColor;
}

void View::setForegroundColor(uint16_t value) {
    this->foregroundColor = value;
}

uint16_t View::getDirectionalAffinity() {
    return this->affinity;
}

void View::setDirectionalAffinity(DirectionalAffinity value) {
    this->affinity = value;
}

std::weak_ptr<View> View::getViewForTouch(Point touch) {
    if (!this->_contains(touch)) {
        // if we don't contain the touch, move on.
        return std::weak_ptr<View>();
    }

    // Convert touch from parent's coordinate system to this view's local coordinate system.
    // Subview frames are expressed in our local coordinate system, so we need to
    // subtract our frame origin (and account for any bounds offset).
    Point localTouch = MakePoint(
        touch.x - this->frame.origin.x + this->bounds.origin.x,
        touch.y - this->frame.origin.y + this->bounds.origin.y
    );

    // Iterate in reverse: last-added subviews are drawn on top (highest z-order)
    // and should receive touch priority first.
    for (auto it = this->subviews.rbegin(); it != this->subviews.rend(); ++it) {
        std::weak_ptr<View> viewForTouch = (*it)->getViewForTouch(localTouch);
        if (viewForTouch.lock()) {
            return viewForTouch;
        }
    }

    // if we end up here, either we have no subviews or the touch didn't touch any of them.
    // return ourselves.
    return this->shared_from_this();
}

Point View::convertPointFromWindow(Point windowPoint) {
    int offsetX = frame.origin.x - bounds.origin.x;
    int offsetY = frame.origin.y - bounds.origin.y;

    std::shared_ptr<View> ancestor = superview.lock();
    while (ancestor) {
        offsetX += ancestor->frame.origin.x - ancestor->bounds.origin.x;
        offsetY += ancestor->frame.origin.y - ancestor->bounds.origin.y;
        ancestor = ancestor->superview.lock();
    }

    return MakePoint(windowPoint.x - offsetX, windowPoint.y - offsetY);
}

bool View::containsPointInWindowCoordinates(Point windowPoint) {
    Point localPoint = this->convertPointFromWindow(windowPoint);
    return (localPoint.x >= 0 && localPoint.x < this->bounds.size.width &&
            localPoint.y >= 0 && localPoint.y < this->bounds.size.height);
}

void View::setNeedsDisplayInRect(Rect rect) {
    std::shared_ptr<View> shared_this = this->shared_from_this();
    std::shared_ptr<View> superview(shared_this);
    while((superview = superview->superview.lock())) {
        rect.origin.x += superview->frame.origin.x;
        rect.origin.y += superview->frame.origin.y;
    }

    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        window->setNeedsDisplayInRect(rect);
    }
}

std::string View::description() {
    char buf[100];
    int status;

    snprintf(buf, sizeof(buf), "<%s: %p; tag = %ld; frame = (%d, %d, %d, %d)>", abi::__cxa_demangle(typeid(*this).name(), 0, 0,&status), this, this->tag, this->frame.origin.x, this->frame.origin.y, this->frame.size.width, this->frame.size.height);

    return std::string(buf);
}

void View::clearTouchChecked() {
    this->_touch_checked = false;
    for(std::shared_ptr<View> view : this->subviews) {
        view.get()->clearTouchChecked();
    }
}

void View::SetDefaultBackgroundColor(uint16_t color) {
    GrayscaleColor::SetDefaultBackgroundColor(color);
}

void View::SetDefaultForegroundColor(uint16_t color) {
    GrayscaleColor::SetDefaultForegroundColor(color);
}

bool View::wantsKeyboardInput() {
    return false;
}

void View::insertText(const std::string& text) {
    // no-op by default
}

void View::deleteBackward() {
    // no-op by default
}

KeyboardType View::keyboardType() {
    return KeyboardTypeDefault;
}

bool View::_contains(Point point) {
    return (
        (this->frame.origin.x <= point.x) && (point.x <= (this->frame.origin.x + this->frame.size.width)) &&
        (this->frame.origin.y <= point.y) && (point.y <= (this->frame.origin.y + this->frame.size.height))
    );
}