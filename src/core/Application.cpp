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
#include "NavigationViewController.hpp"
#include "HatchedView.hpp"
#include "Task.hpp"
#include "Display.hpp"
#include <algorithm>
#include <typeinfo>
#include <cstdlib>
#include "FocusLog.hpp"
#include "esp_timer.h"

static const char *TAG = "Focus";

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
    this->window->setNeedsDisplayInRect(this->window->getFrame());
    while(this->running) {
        for (int i = 0; i < (int)this->tasks.size(); i++) {
            if (this->tasks[i]->run(application)) {
                this->tasks.erase(this->tasks.begin() + i);
                i--;
            }
        }
        this->loopCounter_.fetch_add(1, std::memory_order_relaxed);
    }
}

int32_t Application::detectSwipe(int dx, int dy, int64_t durationUs) {
    int absDx = abs(dx);
    int absDy = abs(dy);
    int64_t maxDuration = 400000;  // 400ms — must be a quick motion
    int minDistance = 60;           // pixels — must travel far enough

    if (durationUs > maxDuration) return 0;

    // Dominant axis must be at least 2x the other to count as directional
    if (absDx >= minDistance && absDx > absDy * 2) {
        return (dx < 0) ? FOCUS_EVENT_SWIPE_LEFT : FOCUS_EVENT_SWIPE_RIGHT;
    }
    if (absDy >= minDistance && absDy > absDx * 2) {
        return (dy < 0) ? FOCUS_EVENT_SWIPE_UP : FOCUS_EVENT_SWIPE_DOWN;
    }
    return 0;
}

void Application::dispatchTouchEvent(Event event) {
    switch (event.type) {
        case FOCUS_EVENT_TOUCH_DOWN:
        {
            this->longPressFired = false;
            this->touchDownTimestamp = event.timestamp;
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
            }
        }
        break;

        case FOCUS_EVENT_TOUCH_MOVED:
        {
            if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                capturedView->handleEvent(event);
            }
        }
        break;

        case FOCUS_EVENT_TOUCH_UP:
        {
            if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                Event upEvent;
                upEvent.userInfo = event.userInfo;
                upEvent.timestamp = event.timestamp;

                if (this->longPressFired) {
                    // Long press already handled; suppress normal tap
                    upEvent.type = FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                } else {
                    Point touchDown = this->window->getTouchDownPoint();
                    Point touchUp = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
                    int64_t duration = event.timestamp - this->touchDownTimestamp;
                    int32_t swipe = this->detectSwipe(touchUp.x - touchDown.x, touchUp.y - touchDown.y, duration);
                    if (swipe) {
                        upEvent.type = swipe;
                    } else {
                        bool isInside = capturedView->containsPointInWindowCoordinates(touchUp);
                        upEvent.type = isInside ? FOCUS_EVENT_TOUCH_UP_INSIDE : FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                    }
                }
                capturedView->handleEvent(upEvent);

                this->window->clearCapturedTouchView();
                this->longPressFired = false;
            }
        }
        break;

        case FOCUS_EVENT_LONG_PRESS:
        {
            this->longPressFired = true;
            if (std::shared_ptr<View> capturedView = this->window->getCapturedTouchView().lock()) {
                capturedView->handleEvent(event);
            }
        }
        break;

        default:
            break;
    }
}

bool Application::handleSystemGestures(Event event) {
    // If a gesture was already recognized, forward events to it
    if (this->recognizedGesture) {
        switch (event.type) {
            case FOCUS_EVENT_TOUCH_MOVED:
                this->recognizedGesture->touchMoved(event);
                return true;
            case FOCUS_EVENT_TOUCH_UP:
                this->recognizedGesture->touchUp(event);
                this->recognizedGesture->reset();
                this->recognizedGesture = nullptr;
                return true;
            case FOCUS_EVENT_LONG_PRESS:
                this->recognizedGesture->longPress(event);
                return true;
            default:
                return false;
        }
    }

    // On TOUCH_DOWN: check which recognizers want this touch
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        Point touch = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
        const auto& recognizers = this->window->getSystemGestureRecognizers();
        this->activeRecognizers.clear();
        for (const auto& recognizer : recognizers) {
            if (recognizer->wantsTouch(touch)) {
                this->activeRecognizers.push_back(recognizer);
            }
        }
        if (this->activeRecognizers.empty()) return false;

        // Start buffering
        this->gestureRecognitionPending = true;
        this->gestureEventBuffer.clear();
        this->gestureEventBuffer.push_back(event);

        for (auto& recognizer : this->activeRecognizers) {
            recognizer->touchDown(event);
        }
        return true;
    }

    // Not in buffering mode — nothing to do
    if (!this->gestureRecognitionPending) return false;

    // Feed event to active recognizers
    for (auto& recognizer : this->activeRecognizers) {
        switch (event.type) {
            case FOCUS_EVENT_TOUCH_MOVED: recognizer->touchMoved(event); break;
            case FOCUS_EVENT_TOUCH_UP:    recognizer->touchUp(event); break;
            case FOCUS_EVENT_LONG_PRESS:  recognizer->longPress(event); break;
            default: break;
        }
    }

    // Check recognizer states
    bool anyRecognized = false;
    bool anyPossible = false;
    for (auto& recognizer : this->activeRecognizers) {
        if (recognizer->getState() == GestureRecognizer::State::Recognized) {
            anyRecognized = true;
            this->recognizedGesture = recognizer;
            break;
        }
        if (recognizer->getState() == GestureRecognizer::State::Possible) {
            anyPossible = true;
        }
    }

    if (anyRecognized) {
        // Gesture won — discard buffer, reset other recognizers
        this->gestureEventBuffer.clear();
        this->gestureRecognitionPending = false;
        for (auto& recognizer : this->activeRecognizers) {
            if (recognizer != this->recognizedGesture) {
                recognizer->reset();
            }
        }
        this->activeRecognizers.clear();
        return true;
    }

    if (anyPossible) {
        // Still undecided — buffer the event
        this->gestureEventBuffer.push_back(event);
        return true;
    }

    // All failed — replay buffered events through normal dispatch
    this->gestureRecognitionPending = false;
    std::vector<Event> buffer = std::move(this->gestureEventBuffer);
    this->activeRecognizers.clear();
    for (const auto& buffered : buffer) {
        this->dispatchTouchEvent(buffered);
    }
    // Also dispatch the current event (it wasn't buffered yet since we check states first)
    this->dispatchTouchEvent(event);
    return true;  // We handled replay
}

void Application::generateEvent(int32_t eventType, int32_t userInfo) {
    Event event;
    event.type = eventType;
    event.userInfo = userInfo;
    event.timestamp = esp_timer_get_time();

    // For touch events, transform native panel coordinates to logical coordinates.
    // Each case is the inverse of the Display's rendering rotation.
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

    if (this->window->touchEnabled) {
        // Give the window first chance to handle (or swallow) any event.
        // For touch events this allows the window to intercept before
        // gesture recognizers and hit-test dispatch run.
        if (this->window->handleEvent(event)) return;

        // Non-touch events were already offered to the window above.
        if (event.type != FOCUS_EVENT_TOUCH_DOWN &&
            event.type != FOCUS_EVENT_TOUCH_MOVED &&
            event.type != FOCUS_EVENT_TOUCH_UP &&
            event.type != FOCUS_EVENT_LONG_PRESS) {
            return;
        }

        if (this->handleSystemGestures(event)) return;
        this->dispatchTouchEvent(event);
    } else if (std::shared_ptr<View> focusedView = this->window->focusedView.lock()) {
        focusedView->handleEvent(event);
    }
}

std::shared_ptr<Window> Application::getWindow() {
    return this->window;
}

void Application::setRootViewController(std::shared_ptr<ViewController> viewController) {
    FOCUS_LOGD(TAG, "setRoot %s", typeid(*viewController).name());
    if (this->rootViewController) {
        // clean up old view controller
        this->rootViewController->viewWillDisappear();
        this->window->removeSubview(this->rootViewController->view);
        this->rootViewController->viewDidDisappear();
    }

    // set up new view controller
    this->rootViewController = viewController;
    this->rootViewController->viewWillAppear();
    this->rootViewController->viewDidLayoutSubviews();
    this->window->addSubview(this->rootViewController->view);
    this->rootViewController->viewDidAppear();
}

bool Application::isModalPresented() const {
    return !this->modalStack.empty();
}

std::shared_ptr<ViewController> Application::activeViewController() const {
    if (!this->modalStack.empty()) {
        return this->modalStack.back().viewController;
    }
    if (auto navVC = std::dynamic_pointer_cast<NavigationViewController>(this->rootViewController)) {
        return navVC->topViewController();
    }
    return this->rootViewController;
}

void Application::presentViewController(std::shared_ptr<ViewController> viewController) {
    FOCUS_LOGD(TAG, "present %s", typeid(*viewController).name());
    ModalEntry entry;
    entry.viewController = viewController;
    entry.previousFocusedView = this->window->focusedView;

    // Create the view so we can inspect it before adding to the window.
    viewController->viewWillAppear();
    viewController->viewDidLayoutSubviews();

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

    // Move focus into the modal's view hierarchy (d-pad/keyboard mode only).
    if (!this->window->isTouchEnabled()) {
        auto descendant = viewController->view->firstFocusableDescendant();
        if (descendant) {
            descendant->becomeFocused();
        }
    }

    this->modalStack.push_back(entry);
}

void Application::dismissViewController() {
    if (this->modalStack.empty()) return;

    ModalEntry entry = this->modalStack.back();
    FOCUS_LOGD(TAG, "dismiss %s", typeid(*entry.viewController).name());
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
    this->window->setNeedsDisplayInRect(this->window->getFrame());
}

void Application::dismissAllViewControllers() {
    while (!this->modalStack.empty()) {
        this->dismissViewController();
    }
}

void Application::quit() {
    this->running = false;
}
