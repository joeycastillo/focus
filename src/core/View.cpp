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
#include <chrono>
#include <cxxabi.h>

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char *VIEW_TAG = "View";
#define VIEW_LOGV(fmt, ...) ESP_LOGV(VIEW_TAG, fmt, ##__VA_ARGS__)
#else
#define VIEW_LOGV(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#endif

int View::drawCount = 0;
int View::cullCount = 0;
int View::fillCount = 0;

View::View(Rect rect) {
    // printf("Creating view %p\n", this);
    this->frame = rect;
    // bounds has origin at (0,0) in the view's local coordinate system
    this->bounds = MakeRect(0, 0, rect.size.width, rect.size.height);
    this->foregroundColor = GrayscaleColor::DefaultForegroundColor();
    this->backgroundColor = GrayscaleColor::DefaultBackgroundColor();
    this->window.reset();
}

View::~View() {
    // printf("Destroying view %p\n", this);
    for (auto& child : this->subviews) {
        child->superview = nullptr;
    }
}

void View::draw(int x, int y, Rect clipRect) {
    if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
        // Spatial culling: skip if entirely outside clip rect.
        if (clipRect.size.width > 0 && clipRect.size.height > 0) {
            Rect screenRect = MakeRect(x + this->frame.origin.x, y + this->frame.origin.y,
                                        this->frame.size.width, this->frame.size.height);
            if (!RectsIntersect(screenRect, clipRect)) {
                cullCount++;
                return;
            }
        }

        drawCount++;

        // Subview coordinates for the occluder scan and draw loop.
        int subviewX = x + this->frame.origin.x - this->bounds.origin.x;
        int subviewY = y + this->frame.origin.y - this->bounds.origin.y;

        // Z-order occlusion culling: if a non-hidden, opaque subview fully
        // covers the clip rect, nothing behind it (parent fill, parent content,
        // earlier siblings) can contribute visible pixels. Skip them.
        int occluderIndex = -1;
        if (clipRect.size.width > 0 && clipRect.size.height > 0) {
            for (int i = (int)this->subviews.size() - 1; i >= 0; i--) {
                auto &child = this->subviews[i];
                if (child->hidden || !child->opaque) continue;
                Rect childScreen = MakeRect(
                    subviewX + child->frame.origin.x,
                    subviewY + child->frame.origin.y,
                    child->frame.size.width,
                    child->frame.size.height);
                if (RectContains(childScreen, clipRect)) {
                    occluderIndex = i;
                    break;
                }
            }
        }

        if (occluderIndex < 0) {
            // No occluder — draw parent content normally.
            auto fillStart = std::chrono::steady_clock::now();
            if (this->opaque) {
                fillCount++;
                display->fillRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->backgroundColor, clipRect);
            }
            auto fillEnd = std::chrono::steady_clock::now();

            auto contentStart = std::chrono::steady_clock::now();
            this->drawContent(x, y, clipRect);
            auto contentEnd = std::chrono::steady_clock::now();

            auto fillUs = std::chrono::duration_cast<std::chrono::microseconds>(fillEnd - fillStart).count();
            auto contentUs = std::chrono::duration_cast<std::chrono::microseconds>(contentEnd - contentStart).count();

            int status;
            char *demangled = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
            VIEW_LOGV("  [draw] %-28s fill=%4lldus  content=%7lldus  frame=(%d,%d %dx%d)",
                (status == 0) ? demangled : typeid(*this).name(),
                (long long)fillUs, (long long)contentUs,
                x + this->frame.origin.x, y + this->frame.origin.y,
                this->frame.size.width, this->frame.size.height);
            if (demangled) free(demangled);
        } else {
            int status;
            char *demangled = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
            VIEW_LOGV("  [draw] %-28s OCCLUDED (by child %d)  frame=(%d,%d %dx%d)",
                (status == 0) ? demangled : typeid(*this).name(),
                occluderIndex,
                x + this->frame.origin.x, y + this->frame.origin.y,
                this->frame.size.width, this->frame.size.height);
            if (demangled) free(demangled);
        }

        // Draw subviews — start from the occluder if one was found.
        int startIdx = (occluderIndex >= 0) ? occluderIndex : 0;
        for (int i = startIdx; i < (int)this->subviews.size(); i++) {
            if (!this->subviews[i]->hidden) {
                this->subviews[i]->draw(subviewX, subviewY, clipRect);
            }
        }
    }
}

void View::drawContent(int x, int y, Rect clipRect) {
    // Base implementation: no custom content.
}

void View::addSubview(std::shared_ptr<View> view) {
    view->superview = this;
    this->subviews.push_back(view);
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        view->setWindow(window);
        window->setNeedsDisplay(true);
    }
}

void View::removeSubview(std::shared_ptr<View> view) {
    // Check if the focused view lives inside the subtree being removed.
    bool removingFocused = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        std::shared_ptr<View> focused = window->getFocusedView().lock();
        if (focused) {
            View* v = focused.get();
            while (v) {
                if (v == view.get()) {
                    removingFocused = true;
                    focused->willResignFocus();
                    focused->focused = false;
                    window->focusedView.reset();
                    focused->didResignFocus();
                    break;
                }
                v = v->superview;
            }
        }
    }

    view->superview = nullptr;
    view->window.reset();
    int index = std::distance(this->subviews.begin(), std::find(this->subviews.begin(), this->subviews.end(), view));
    this->subviews.erase(this->subviews.begin() + index);
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        if (removingFocused) {
            window->becomeFocused();
        }
        window->setNeedsDisplay(true);
    }
}

bool View::isFocused() {
    return this->focused;
}

bool View::canBecomeFocused() {
    return false;
}

std::shared_ptr<View> View::firstFocusableDescendant() {
    for (auto& child : this->subviews) {
        if (child->hidden) continue;
        if (child->canBecomeFocused()) return child;
        auto found = child->firstFocusableDescendant();
        if (found) return found;
    }
    return nullptr;
}

std::shared_ptr<View> View::lastFocusableDescendant() {
    for (auto it = this->subviews.rbegin(); it != this->subviews.rend(); ++it) {
        if ((*it)->hidden) continue;
        if ((*it)->canBecomeFocused()) return *it;
        auto found = (*it)->lastFocusableDescendant();
        if (found) return found;
    }
    return nullptr;
}

int View::indexOfChildContaining(std::shared_ptr<View> view) {
    for (int i = 0; i < (int)this->subviews.size(); i++) {
        View* v = view.get();
        while (v) {
            if (v == this->subviews[i].get()) return i;
            v = v->superview;
        }
    }
    return -1;
}

bool View::becomeFocused() {
    if (this->hidden) return false;
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
        if (this->superview) {
            // when resigning focus (due to being removed from a superview), pass focus to the superview.
            this->superview->becomeFocused();
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
    if (this->superview) {
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
        }
    }
}

void View::willResignFocus() {
    // nothing to do here
}

void View::didResignFocus() {
    if (this->superview) {
        if (std::shared_ptr<Window> window = this->getWindow().lock()) {
            this->setNeedsDisplayInRect(this->frame);
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

    auto it = this->actions.find(event.type);
    if (it != this->actions.end()) {
        auto &owned = it->second;
        if (owned.owner.has_value() && owned.owner->expired()) {
            // Owner was destroyed; remove the orphaned action and fall through.
            this->actions.erase(it);
        } else {
            // Action is live — invoke it.
            if (std::shared_ptr<Application> application = window->application.lock()) {
                owned.callback(event, this->shared_from_this());
            }
            return true; // consumed — don't bubble
        }
    }
    {
        // otherwise, some events are handled internally
        switch (event.type) {
            case FOCUS_EVENT_DIRECTION_LEFT:
            case FOCUS_EVENT_DIRECTION_DOWN:
            case FOCUS_EVENT_DIRECTION_UP:
            case FOCUS_EVENT_DIRECTION_RIGHT:
            {
                // Find which direct child contains (or is) the focused view.
                int index = this->indexOfChildContaining(focusedView);
                if (index < 0) break; // focused view is not in our subtree; let it bubble

                // Determine if this direction maps to "next" or "previous" for our affinity.
                bool isNext = false;
                bool isRelevant = false;
                if (this->affinity == DirectionalAffinityVertical) {
                    if (event.type == FOCUS_EVENT_DIRECTION_DOWN) { isNext = true; isRelevant = true; }
                    else if (event.type == FOCUS_EVENT_DIRECTION_UP) { isNext = false; isRelevant = true; }
                } else if (this->affinity == DirectionalAffinityHorizontal) {
                    if (event.type == FOCUS_EVENT_DIRECTION_RIGHT) { isNext = true; isRelevant = true; }
                    else if (event.type == FOCUS_EVENT_DIRECTION_LEFT) { isNext = false; isRelevant = true; }
                }
                if (!isRelevant) break; // cross-axis direction; let it bubble

                if (isNext) {
                    for (int i = index + 1; i < (int)this->subviews.size(); i++) {
                        if (this->subviews[i]->hidden) continue;
                        if (this->subviews[i]->canBecomeFocused()) {
                            this->subviews[i]->becomeFocused();
                            return true;
                        }
                        auto descendant = this->subviews[i]->firstFocusableDescendant();
                        if (descendant) {
                            descendant->becomeFocused();
                            return true;
                        }
                    }
                } else {
                    for (int i = index - 1; i >= 0; i--) {
                        if (this->subviews[i]->hidden) continue;
                        if (this->subviews[i]->canBecomeFocused()) {
                            this->subviews[i]->becomeFocused();
                            return true;
                        }
                        auto descendant = this->subviews[i]->lastFocusableDescendant();
                        if (descendant) {
                            descendant->becomeFocused();
                            return true;
                        }
                    }
                }
                // Ran out of siblings — let it bubble to the parent.
                break;
            }
            case FOCUS_EVENT_SELECT:
            {
                // If no SELECT action was registered (checked above), fall back
                // to TOUCH_UP_INSIDE. This makes buttons and cells that only
                // register touch actions work with d-pad/keyboard SELECT.
                auto fallback = this->actions.find(FOCUS_EVENT_TOUCH_UP_INSIDE);
                if (fallback != this->actions.end()) {
                    auto &owned = fallback->second;
                    if (owned.owner.has_value() && owned.owner->expired()) {
                        this->actions.erase(fallback);
                    } else {
                        if (std::shared_ptr<Application> application = window->application.lock()) {
                            owned.callback(event, this->shared_from_this());
                        }
                        return true;
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    if (this->clipsFocus) {
        // This view traps focus — swallow the event so it doesn't escape.
        return true;
    }

    if (this->superview) {
        // if the event was not handled internally, bubble it up to the next view in the hierarchy.
        this->superview->handleEvent(event);
    }

    return false;
}

void View::setAction(const Action &action, int32_t type) {
    this->actions[type] = {action, std::nullopt};
}

void View::setAction(const Action &action, int32_t type, std::weak_ptr<void> owner) {
    this->actions[type] = {action, owner};
}

void View::removeAction(int32_t type) {
    this->actions.erase(type);
}

View* View::getSuperview() {
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
    if (this->opaque == value) return;

    this->opaque = value;
    this->appearanceDidChange();
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
    this->appearanceDidChange();
}

uint16_t View::getForegroundColor() {
    return this->foregroundColor;
}

void View::setForegroundColor(uint16_t value) {
    this->foregroundColor = value;
    this->appearanceDidChange();
}

void View::appearanceDidChange() {
    // Default: nothing. Subclasses with rendering caches override this.
}

uint16_t View::getDirectionalAffinity() {
    return this->affinity;
}

void View::setDirectionalAffinity(DirectionalAffinity value) {
    this->affinity = value;
}

bool View::getClipsFocus() const {
    return this->clipsFocus;
}

void View::setClipsFocus(bool value) {
    this->clipsFocus = value;
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

    View* ancestor = superview;
    while (ancestor) {
        offsetX += ancestor->frame.origin.x - ancestor->bounds.origin.x;
        offsetY += ancestor->frame.origin.y - ancestor->bounds.origin.y;
        ancestor = ancestor->superview;
    }

    return MakePoint(windowPoint.x - offsetX, windowPoint.y - offsetY);
}

bool View::containsPointInWindowCoordinates(Point windowPoint) {
    Point localPoint = this->convertPointFromWindow(windowPoint);
    return (localPoint.x >= 0 && localPoint.x < this->bounds.size.width &&
            localPoint.y >= 0 && localPoint.y < this->bounds.size.height);
}

void View::setNeedsDisplayInRect(Rect rect) {
    View* sv = this->superview;
    while (sv) {
        rect.origin.x += sv->frame.origin.x;
        rect.origin.y += sv->frame.origin.y;
        sv = sv->superview;
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