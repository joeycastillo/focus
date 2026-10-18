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
#include "FocusMetrics.hpp"
#include "View.hpp"
#include "Display.hpp"
#include "KeyboardView.hpp"
#include "ViewController.hpp"
#include "HatchedView.hpp"
#include "NavigationViewController.hpp"
#include <algorithm>
#include <typeinfo>
#include "FocusLog.hpp"

namespace focus {

namespace {

// True if the view is the ancestor or lies inside it.
bool isWithin(const View* view, const View* ancestor) {
    for (const View* v = view; v; v = v->getSuperview()) {
        if (v == ancestor) return true;
    }
    return false;
}

}  // namespace

static const char *TAG = "Focus";

Window::Window(std::shared_ptr<Display> display, Size size) : View(MakeRect(0, 0, size.width, size.height)) {
    this->display = display;
    this->setNeedsDisplayInRect(this->frame);
}

void Window::addSubview(std::shared_ptr<View> view) {
    view->setWindow(std::static_pointer_cast<Window>(this->shared_from_this()));
    View::addSubview(view);
    // Touch windows start latent; don't auto-focus descendants.
    if (this->touchEnabled) return;
    // If nothing meaningful is focused, focus the first focusable descendant.
    std::shared_ptr<View> currentFocus = this->focusedView.lock();
    bool windowIsFocused = (currentFocus.get() == this);
    if (!currentFocus || windowIsFocused) {
        if (!view->becomeFocused()) {
            std::shared_ptr<View> descendant = view->firstFocusableDescendant();
            if (descendant) {
                descendant->becomeFocused();
            }
        }
    }
}

bool Window::canBecomeFocused() const {
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

void Window::clearNeedsDisplay() {
    this->dirty = false;
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

    // Clamp to window bounds so downstream drawing never receives
    // an oversized dirty rect from accumulated view offsets.
    int right = finalRect.origin.x + finalRect.size.width;
    int bottom = finalRect.origin.y + finalRect.size.height;
    if (finalRect.origin.x < 0) finalRect.origin.x = 0;
    if (finalRect.origin.y < 0) finalRect.origin.y = 0;
    if (right > this->frame.size.width) right = this->frame.size.width;
    if (bottom > this->frame.size.height) bottom = this->frame.size.height;
    finalRect.size.width = right - finalRect.origin.x;
    finalRect.size.height = bottom - finalRect.origin.y;
    if (finalRect.size.width <= 0 || finalRect.size.height <= 0) return;

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

bool Window::isFocusEngaged() {
    std::shared_ptr<View> focused = this->focusedView.lock();
    return focused != nullptr && focused.get() != this;
}

std::weak_ptr<Window> Window::getWindow() const {
    // A Window is its own window, so this returns a handle to itself. The const method
    // hands that back read-only, so the casts convert it into a normal, usable window.
    return std::static_pointer_cast<Window>(
        std::const_pointer_cast<View>(this->shared_from_this()));
}

void Window::setWindow(std::shared_ptr<Window> window) {
    // nothing to do here
}

Rect Window::getContentRect() {
    return this->getFrame();
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
    this->touchDownPoint = PointZero;
}

Point Window::getTouchDownPoint() {
    return this->touchDownPoint;
}

void Window::addSystemGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    this->systemGestureRecognizers.push_back(recognizer);
}

void Window::removeSystemGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    auto it = std::find(this->systemGestureRecognizers.begin(), this->systemGestureRecognizers.end(), recognizer);
    if (it != this->systemGestureRecognizers.end()) {
        this->systemGestureRecognizers.erase(it);
    }
}

const std::vector<std::shared_ptr<GestureRecognizer>>& Window::getSystemGestureRecognizers() const {
    return this->systemGestureRecognizers;
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
    std::shared_ptr<View> focused = this->focusedView.lock();
    if (!focused) return;

    KeyboardType requestedType = focused->keyboardType();

    // If keyboard exists but has wrong type, dismiss it first
    if (this->keyboard && this->currentKeyboardType != requestedType) {
        dismissKeyboard();
    }

    if (this->keyboard) return;

    int keyboardHeight = FocusMetrics::get().keyboardHeight;
    this->keyboard = std::make_shared<KeyboardView>(
        MakeRect(0, this->frame.size.height - keyboardHeight,
                 this->frame.size.width, keyboardHeight),
        requestedType);
    this->keyboard->setKeyCallback(
        std::bind(&Window::onKeyPressed, this, std::placeholders::_1));
    this->currentKeyboardType = requestedType;
    // Use View::addSubview to add the keyboard directly to the window,
    // bypassing any subclass overrides.
    View::addSubview(this->keyboard);
}

void Window::dismissKeyboard() {
    if (!this->keyboard) return;

    std::shared_ptr<KeyboardView> kb = this->keyboard;
    this->keyboard.reset();
    this->currentKeyboardType = KeyboardType::Default;
    View::removeSubview(kb);
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

    View* v = view.get();
    while (v) {
        if (v == this->keyboard.get()) return true;
        v = v->superview;
    }
    return false;
}

void Window::setRootViewController(std::shared_ptr<ViewController> viewController) {
    FOCUS_LOGD(TAG, "setRoot %s", typeid(*viewController).name());

    // Remove the current modals, top first, revealing nothing.
    std::vector<ModalEntry> modals;
    modals.swap(this->modalStack);
    std::shared_ptr<ViewController> oldRoot = this->rootViewController;
    bool oldRootHidden = this->rootHidden;
    this->rootViewController = nullptr;
    for (auto it = modals.rbegin(); it != modals.rend(); ++it) {
        this->removeModal(*it);
    }
    if (oldRoot && !oldRootHidden) {
        this->hideViewController(oldRoot);
    }

    // A root set from a callback during teardown is replaced too.
    if (this->rootViewController && !this->rootHidden) {
        this->hideViewController(this->rootViewController);
    }

    // Show the new root unless a modal presented during teardown covers it.
    this->rootViewController = viewController;
    this->rootHidden = true;
    this->updateCoverage();
}

std::shared_ptr<ViewController> Window::getRootViewController() const {
    return this->rootViewController;
}

bool Window::isModalPresented() const {
    return !this->modalStack.empty();
}

std::shared_ptr<ViewController> Window::activeViewController() const {
    if (!this->modalStack.empty()) {
        return this->modalStack.back().viewController;
    }
    if (auto navVC = std::dynamic_pointer_cast<NavigationViewController>(this->rootViewController)) {
        return navVC->topViewController();
    }
    return this->rootViewController;
}

void Window::presentViewController(std::shared_ptr<ViewController> viewController) {
    FOCUS_LOGD(TAG, "present %s", typeid(*viewController).name());
    bool wasEngaged = this->isFocusEngaged();
    ModalEntry entry;
    entry.viewController = viewController;
    entry.previousFocusedView = this->focusedView;

    // Create the view so we can inspect it before adding to the window.
    viewController->setWindow(std::static_pointer_cast<Window>(this->shared_from_this()));
    viewController->viewWillAppear();
    viewController->viewDidLayoutSubviews();

    // Add a dimmer overlay unless the modal's view is opaque (full-screen).
    if (!viewController->view->isOpaque()) {
        Size windowSize = this->getFrame().size;
        int blackColor = this->getForegroundColor();
        entry.dimmer = std::make_shared<HatchedView>(
            MakeRect(0, 0, windowSize.width, windowSize.height), blackColor);
        this->addSubview(entry.dimmer);
    }

    // Present the modal VC's view on top. Clip focus so d-pad
    // navigation cannot escape the modal into the views behind it.
    viewController->view->setClipsFocus(true);
    this->addSubview(viewController->view);
    this->modalStack.push_back(entry);

    // An opaque modal hides what it covers.
    this->updateCoverage();

    // Stop if a callback covered or dismissed this modal.
    auto it = std::find_if(this->modalStack.begin(), this->modalStack.end(),
        [&](const ModalEntry& e) { return e.viewController == viewController; });
    if (it == this->modalStack.end() || it->hidden) return;
    viewController->viewDidAppear();

    // A modal presented above this one already moved focus.
    if (this->modalStack.back().viewController != viewController) return;

    // Move focus into the modal when focus is driving; never leave it behind the dimmer.
    if (!this->isTouchEnabled() || wasEngaged) {
        auto descendant = viewController->view->firstFocusableDescendant();
        if (descendant) {
            descendant->becomeFocused();
        } else if (this->isTouchEnabled()) {
            this->becomeFocused();
        }
    }
}

void Window::dismissViewController() {
    if (this->modalStack.empty()) return;

    ModalEntry entry = this->modalStack.back();
    FOCUS_LOGD(TAG, "dismiss %s", typeid(*entry.viewController).name());
    this->modalStack.pop_back();

    // Removing the modal's view resets focus to the window; capture first.
    bool wasEngaged = this->isFocusEngaged();

    this->removeModal(entry);
    this->updateCoverage();
    this->restoreFocus(entry.previousFocusedView, wasEngaged);

    // Mark full window as needing display
    this->setNeedsDisplayInRect(this->getFrame());
}

void Window::dismissAllViewControllers() {
    if (this->modalStack.empty()) return;

    // Remove the current modals, top first, revealing nothing.
    std::vector<ModalEntry> modals;
    modals.swap(this->modalStack);
    bool wasEngaged = this->isFocusEngaged();
    for (auto it = modals.rbegin(); it != modals.rend(); ++it) {
        this->removeModal(*it);
    }
    this->updateCoverage();
    this->restoreFocus(modals.front().previousFocusedView, wasEngaged);
    this->setNeedsDisplayInRect(this->getFrame());
}

void Window::updateCoverage() {
    // A nested call returns early; the outer loop picks up its changes.
    if (this->updatingCoverage) return;
    this->updatingCoverage = true;

    // Change one controller per pass, then rescan.
    bool changed = true;
    while (changed) {
        changed = false;
        bool covered = false;
        std::shared_ptr<View> above;
        for (size_t i = this->modalStack.size(); i-- > 0;) {
            ModalEntry& entry = this->modalStack[i];
            if (entry.hidden != covered) {
                entry.hidden = covered;
                if (entry.dimmer) entry.dimmer->setHidden(covered);
                if (covered) {
                    this->hideViewController(entry.viewController);
                } else {
                    this->showViewController(entry.viewController, above, true);
                }
                changed = true;
                break;
            }
            if (entry.dimmer) {
                above = entry.dimmer;
            } else {
                above = entry.viewController->view;
                covered = true;
            }
        }
        if (!changed && this->rootViewController && this->rootHidden != covered) {
            this->rootHidden = covered;
            if (covered) {
                this->hideViewController(this->rootViewController);
            } else {
                this->showViewController(this->rootViewController, above, false);
            }
            changed = true;
        }
    }

    this->updatingCoverage = false;
}

void Window::hideViewController(std::shared_ptr<ViewController> viewController) {
    viewController->viewWillDisappear();
    if (viewController->view) {
        this->removeSubview(viewController->view);
    }
    viewController->viewDidDisappear();
}

void Window::showViewController(std::shared_ptr<ViewController> viewController,
                                std::shared_ptr<View> above, bool clipsFocus) {
    viewController->setWindow(std::static_pointer_cast<Window>(this->shared_from_this()));
    viewController->viewWillAppear();
    viewController->viewDidLayoutSubviews();
    if (viewController->view) {
        if (clipsFocus) viewController->view->setClipsFocus(true);
        const auto& subviews = this->getSubviews();
        auto it = std::find(subviews.begin(), subviews.end(), above);
        if (!above || it == subviews.end()) {
            this->addSubview(viewController->view);
        } else {
            this->insertSubview(viewController->view, it - subviews.begin());
        }
    }
    viewController->viewDidAppear();
}

void Window::removeModal(const ModalEntry& entry) {
    if (!entry.hidden) {
        this->hideViewController(entry.viewController);
    }
    if (entry.dimmer) {
        this->removeSubview(entry.dimmer);
    }
}

void Window::restoreFocus(std::weak_ptr<View> previousFocusedView, bool wasEngaged) {
    // Leave focus latent if the user went latent inside the modal.
    if (this->isTouchEnabled() && !wasEngaged) return;

    // The view focused before the modal, if it is inside whatever is on top now.
    auto top = this->modalStack.empty() ? this->rootViewController
                                        : this->modalStack.back().viewController;
    std::shared_ptr<View> target = previousFocusedView.lock();
    if (target && !(top && top->view && isWithin(target.get(), top->view.get()))) target = nullptr;

    // Otherwise the first focusable view on top: the active content, then its container.
    if (!target) {
        auto active = this->activeViewController();
        if (active && active->view) target = active->view->firstFocusableDescendant();
    }
    if (!target && top && top->view) target = top->view->firstFocusableDescendant();
    if (target) target->becomeFocused();
}

void Window::summonFocus(bool fromEnd) {
    // Summon domain: the top modal's view when presented, else the window.
    std::shared_ptr<View> domain = this->modalStack.empty()
        ? this->shared_from_this()
        : this->modalStack.back().viewController->view;
    if (!domain) return;

    // Prefer the active view controller's content within the domain.
    std::shared_ptr<View> target;
    std::shared_ptr<ViewController> vc = this->activeViewController();
    if (vc && vc->view) {
        target = fromEnd ? vc->view->lastFocusableDescendant()
                         : vc->view->firstFocusableDescendant();
    }
    if (!target) {
        target = fromEnd ? domain->lastFocusableDescendant()
                         : domain->firstFocusableDescendant();
    }
    if (target) target->becomeFocused();
}

}  // namespace focus
