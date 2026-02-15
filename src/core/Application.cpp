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

#include "Application.hpp"
#include "ViewController.hpp"
#include "HatchedView.hpp"
#include "Task.hpp"
#include "Display.hpp"
#include <algorithm>

Application::Application(const std::shared_ptr<Window>& window) {
    this->window = window;
}

void Application::addTask(std::shared_ptr<Task> task) {
    this->tasks.push_back(task);
}

void Application::run() {
    this->setup();
    std::shared_ptr<Application> application = this->shared_from_this();
    this->window->application = application;
    // Only focus the window if nothing was focused during setup().
    if (!this->window->focusedView.lock()) {
        this->window->becomeFocused();
    }
    this->window->setNeedsDisplay(true);
    while(this->running) {
        for (int i = 0; i < (int)this->tasks.size(); i++) {
            if (this->tasks[i]->run(application)) {
                this->tasks.erase(this->tasks.begin() + i);
                i--;
            }
        }
    }
}

void Application::generateEvent(int32_t eventType, int32_t userInfo) {
    Event event;
    event.type = eventType;
    event.userInfo = userInfo;

    // For touch events, transform native panel coordinates to logical coordinates.
    // Each case is the inverse of the rendering rotation in EPaperDisplay.
    if (eventType == FOCUS_EVENT_TOUCH_DOWN ||
        eventType == FOCUS_EVENT_TOUCH_MOVED ||
        eventType == FOCUS_EVENT_TOUCH_UP) {
        if (auto display = this->window->getDisplay().lock()) {
            int nx = userInfo >> 16;
            int ny = userInfo & 0xFFFF;
            int lx, ly;
            int nw = display->getNativeWidth();
            int nh = display->getNativeHeight();
            switch (display->getRotation()) {
                case 0:  lx = nx;       ly = ny;       break;
                case 1:  lx = ny;       ly = nw-1-nx;  break;
                case 2:  lx = nw-1-nx;  ly = nh-1-ny;  break;
                case 3:  lx = nh-1-ny;  ly = nx;       break;
                default: lx = nx;       ly = ny;       break;
            }
            event.userInfo = (lx << 16) | ly;
        }
    }

    if (this->window.get()->touchEnabled) {
        switch (event.type) {
            case FOCUS_EVENT_TOUCH_DOWN:
            {
                longPressFired = false;
                Point touch = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
                if (std::shared_ptr<View> touchedView = this->window->getViewForTouch(touch).lock()) {
                    // If a text-input view is focused and the touch landed outside
                    // both it and the keyboard, resign focus to dismiss the keyboard.
                    if (std::shared_ptr<View> focused = this->window->getFocusedView().lock()) {
                        if (focused->wantsKeyboardInput() &&
                            touchedView != focused &&
                            !this->window->isKeyboardView(touchedView)) {
                            this->window->becomeFocused();
                        }
                    }
                    this->window->setCapturedTouchView(touchedView, touch);
                    touchedView->handleEvent(event);
                    return;
                }
            }
            break;

            case FOCUS_EVENT_TOUCH_MOVED:
            {
                if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                    capturedView->handleEvent(event);
                    return;
                }
            }
            break;

            case FOCUS_EVENT_TOUCH_UP:
            {
                if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                    Event upEvent;
                    upEvent.userInfo = event.userInfo;

                    if (longPressFired) {
                        // Long press already handled; suppress normal tap
                        upEvent.type = FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                    } else {
                        Point touchUp = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
                        bool isInside = capturedView->containsPointInWindowCoordinates(touchUp);
                        upEvent.type = isInside ? FOCUS_EVENT_TOUCH_UP_INSIDE : FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                    }
                    capturedView->handleEvent(upEvent);

                    this->window->clearCapturedTouchView();
                    longPressFired = false;
                    return;
                }
            }
            break;

            case FOCUS_EVENT_LONG_PRESS:
            {
                longPressFired = true;
                if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                    capturedView->handleEvent(event);
                    return;
                }
            }
            break;

            default:
                // Non-touch events: deliver to the window
                this->window->handleEvent(event);
                return;
        }
    } else if (std::shared_ptr<View> focusedView = this->window->focusedView.lock()) {
        focusedView->handleEvent(event);
    }
}

std::shared_ptr<Window> Application::getWindow() {
    return this->window;
}

void Application::setRootViewController(std::shared_ptr<ViewController> viewController) {
    if (this->rootViewController) {
        // clean up old view controller
        this->rootViewController->viewWillDisappear();
        this->window->removeSubview(this->rootViewController->view);
        this->rootViewController->viewDidDisappear();
    }

    // set up new view controller
    this->rootViewController = viewController;
    this->rootViewController->viewWillAppear();
    this->window->addSubview(this->rootViewController->view);
    this->rootViewController->viewDidAppear();
}

bool Application::isModalPresented() const {
    return !this->modalStack.empty();
}

void Application::presentViewController(std::shared_ptr<ViewController> viewController) {
    ModalEntry entry;
    entry.viewController = viewController;
    entry.previousFocusedView = this->window->focusedView;

    // Create the view so we can inspect it before adding to the window.
    viewController->viewWillAppear();

    // Add a dimmer overlay unless the modal's view is opaque (full-screen).
    if (!viewController->view->isOpaque()) {
        Size windowSize = this->window->getFrame().size;
        int blackColor = this->window->getForegroundColor();
        entry.dimmer = std::make_shared<HatchedView>(
            MakeRect(0, 0, windowSize.width, windowSize.height), blackColor);
        this->window->addSubview(entry.dimmer);
    }

    // Present the modal VC's view on top. Clip focus so d-pad
    // navigation cannot escape the modal into the views behind it.
    viewController->view->setClipsFocus(true);
    this->window->addSubview(viewController->view);
    viewController->viewDidAppear();

    // Move focus into the modal's view hierarchy.
    auto descendant = viewController->view->firstFocusableDescendant();
    if (descendant) {
        descendant->becomeFocused();
    }

    this->modalStack.push_back(entry);
}

void Application::dismissViewController() {
    if (this->modalStack.empty()) return;

    ModalEntry entry = this->modalStack.back();
    this->modalStack.pop_back();

    // Remove modal VC's view
    entry.viewController->viewWillDisappear();
    this->window->removeSubview(entry.viewController->view);
    entry.viewController->viewDidDisappear();

    // Remove dimmer (absent for full-screen modals)
    if (entry.dimmer) {
        this->window->removeSubview(entry.dimmer);
    }

    // Restore previous focus
    if (auto previousView = entry.previousFocusedView.lock()) {
        previousView->becomeFocused();
    }

    // Mark full window as needing display
    this->window->setNeedsDisplay(true);
}

void Application::dismissAllViewControllers() {
    while (!this->modalStack.empty()) {
        this->dismissViewController();
    }
}

void Application::quit() {
    this->running = false;
}
