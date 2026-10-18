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
#include "Task.hpp"
#include "Display.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "focus_platform.h"

namespace focus {

Application::Application(const std::shared_ptr<Window>& window) {
    this->window = window;
    this->windows.push_back(window);
}

void Application::addWindow(std::shared_ptr<Window> window) {
    if (!window) return;
    this->windows.push_back(window);
    if (!this->weak_from_this().expired()) {
        window->application = this->shared_from_this();
    }
}

void Application::addTask(std::shared_ptr<Task> task) {
    this->tasks.push_back(task);
}

void Application::run() {
    assert(!this->weak_from_this().expired() && "Application must be managed by std::shared_ptr (use std::make_shared)");
    std::shared_ptr<Application> application = this->shared_from_this();
    for (auto& w : this->windows) w->application = application;
    this->setup();
    // A window added during setup() is already bound; this is a safety net.
    for (auto& w : this->windows) w->application = application;
    // Only focus the window if nothing was focused during setup().
    if (!this->window->focusedView.lock()) {
        this->window->becomeFocused();
    }
    for (auto& w : this->windows) w->setNeedsDisplayInRect(w->getFrame());
    // With no tasks left, nothing can ever call quit(), so an empty list ends the loop.
    while(this->running && !this->tasks.empty()) {
        for (int i = 0; i < (int)this->tasks.size(); i++) {
            if (this->tasks[i]->run(application)) {
                this->tasks.erase(this->tasks.begin() + i);
                i--;
            }
        }
        this->loopCounter_.fetch_add(1, std::memory_order_relaxed);
    }
}

bool Application::anyTaskBusy() const {
    for (auto& task : this->tasks) {
        if (task->isBusy()) return true;
    }
    return false;
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

void Application::dispatchTouchEvent(Event event, const std::shared_ptr<Window>& window) {
    switch (event.type) {
        case FOCUS_EVENT_TOUCH_DOWN:
        {
            window->longPressFired = false;
            window->touchDownTimestamp = event.timestamp;
            Point touch = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
            if (std::shared_ptr<View> touchedView = window->getViewForTouch(touch).lock()) {
                // A touch outside both the focused view and the keyboard
                // returns focus to the window (and dismisses any keyboard).
                if (std::shared_ptr<View> focused = window->getFocusedView().lock()) {
                    if (focused.get() != window.get() &&
                        !focused->containsPointInWindowCoordinates(touch) &&
                        !window->isKeyboardView(touchedView)) {
                        window->becomeFocused();
                    }
                }
                window->setCapturedTouchView(touchedView, touch);
                touchedView->handleEvent(event);
            }
        }
        break;

        case FOCUS_EVENT_TOUCH_MOVED:
        {
            if (std::shared_ptr<View> capturedView = window->getCapturedTouchView().lock()) {
                capturedView->handleEvent(event);
            }
        }
        break;

        case FOCUS_EVENT_TOUCH_UP:
        {
            if (std::shared_ptr<View> capturedView = window->getCapturedTouchView().lock()) {
                Event upEvent;
                upEvent.userInfo = event.userInfo;
                upEvent.timestamp = event.timestamp;

                if (window->longPressFired) {
                    // Long press already handled; suppress normal tap
                    upEvent.type = FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                } else {
                    Point touchDown = window->getTouchDownPoint();
                    Point touchUp = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
                    int64_t duration = event.timestamp - window->touchDownTimestamp;
                    int32_t swipe = this->detectSwipe(touchUp.x - touchDown.x, touchUp.y - touchDown.y, duration);
                    if (swipe) {
                        upEvent.type = swipe;
                    } else {
                        bool isInside = capturedView->containsPointInWindowCoordinates(touchUp);
                        upEvent.type = isInside ? FOCUS_EVENT_TOUCH_UP_INSIDE : FOCUS_EVENT_TOUCH_UP_OUTSIDE;
                    }
                }
                capturedView->handleEvent(upEvent);

                window->clearCapturedTouchView();
                window->longPressFired = false;
            }
        }
        break;

        case FOCUS_EVENT_LONG_PRESS:
        {
            window->longPressFired = true;
            if (std::shared_ptr<View> capturedView = window->getCapturedTouchView().lock()) {
                capturedView->handleEvent(event);
            }
        }
        break;

        default:
            break;
    }
}

bool Application::handleSystemGestures(Event event, const std::shared_ptr<Window>& window) {
    // If a gesture was already recognized, forward events to it
    if (window->recognizedGesture) {
        switch (event.type) {
            case FOCUS_EVENT_TOUCH_MOVED:
                window->recognizedGesture->touchMoved(event);
                return true;
            case FOCUS_EVENT_TOUCH_UP:
                window->recognizedGesture->touchUp(event);
                window->recognizedGesture->reset();
                window->recognizedGesture = nullptr;
                return true;
            case FOCUS_EVENT_LONG_PRESS:
                window->recognizedGesture->longPress(event);
                return true;
            default:
                return false;
        }
    }

    // On TOUCH_DOWN: check which recognizers want this touch
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        Point touch = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
        const auto& recognizers = window->getSystemGestureRecognizers();
        window->activeRecognizers.clear();
        for (const auto& recognizer : recognizers) {
            if (recognizer->wantsTouch(touch)) {
                window->activeRecognizers.push_back(recognizer);
            }
        }
        if (window->activeRecognizers.empty()) return false;

        // Start buffering
        window->gestureRecognitionPending = true;
        window->gestureEventBuffer.clear();
        window->gestureEventBuffer.push_back(event);

        for (auto& recognizer : window->activeRecognizers) {
            recognizer->touchDown(event);
        }
        return true;
    }

    // Not in buffering mode — nothing to do
    if (!window->gestureRecognitionPending) return false;

    // Feed event to active recognizers
    for (auto& recognizer : window->activeRecognizers) {
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
    for (auto& recognizer : window->activeRecognizers) {
        if (recognizer->getState() == GestureRecognizer::State::Recognized) {
            anyRecognized = true;
            window->recognizedGesture = recognizer;
            break;
        }
        if (recognizer->getState() == GestureRecognizer::State::Possible) {
            anyPossible = true;
        }
    }

    if (anyRecognized) {
        // Gesture won — discard buffer, reset other recognizers
        window->gestureEventBuffer.clear();
        window->gestureRecognitionPending = false;
        for (auto& recognizer : window->activeRecognizers) {
            if (recognizer != window->recognizedGesture) {
                recognizer->reset();
            }
        }
        window->activeRecognizers.clear();
        return true;
    }

    if (anyPossible) {
        // Still undecided — buffer the event
        window->gestureEventBuffer.push_back(event);
        return true;
    }

    // All failed — replay buffered events through normal dispatch
    window->gestureRecognitionPending = false;
    std::vector<Event> buffer = std::move(window->gestureEventBuffer);
    window->activeRecognizers.clear();
    for (const auto& buffered : buffer) {
        this->dispatchTouchEvent(buffered, window);
    }
    // Also dispatch the current event (it wasn't buffered yet since we check states first)
    this->dispatchTouchEvent(event, window);
    return true;  // We handled replay
}

bool Application::isFocusNavigationEvent(int32_t type) {
    switch (type) {
        case FOCUS_EVENT_DIRECTION_LEFT:
        case FOCUS_EVENT_DIRECTION_DOWN:
        case FOCUS_EVENT_DIRECTION_UP:
        case FOCUS_EVENT_DIRECTION_RIGHT:
        case FOCUS_EVENT_SELECT:
        case FOCUS_EVENT_BACK:
        case FOCUS_EVENT_FORWARD:
        case FOCUS_EVENT_ACCESSIBILITY_NEXT:
        case FOCUS_EVENT_ACCESSIBILITY_PREVIOUS:
            return true;
        default:
            return false;
    }
}

void Application::handleLatentNavigationEvent(Event event, const std::shared_ptr<Window>& window) {
    switch (event.type) {
        case FOCUS_EVENT_DIRECTION_LEFT:
        case FOCUS_EVENT_DIRECTION_DOWN:
        case FOCUS_EVENT_DIRECTION_UP:
        case FOCUS_EVENT_DIRECTION_RIGHT:
            window->summonFocus(false);
            break;
        case FOCUS_EVENT_ACCESSIBILITY_NEXT:
            window->summonFocus(false);
            break;
        case FOCUS_EVENT_ACCESSIBILITY_PREVIOUS:
            window->summonFocus(true);
            break;
        case FOCUS_EVENT_BACK:
        case FOCUS_EVENT_FORWARD:
        {
            std::shared_ptr<ViewController> vc = window->activeViewController();
            if (vc && vc->view) {
                vc->view->handleEvent(event);
            }
            break;
        }
        default:
            // SELECT: nothing is focused, so there is nothing to activate.
            break;
    }
}

void Application::generateEvent(int32_t eventType, int32_t userInfo) {
    this->generateEvent(eventType, userInfo, this->window);
}

void Application::generateEvent(int32_t eventType, int32_t userInfo, const std::shared_ptr<Window>& target) {
    // Copy, so a handler that calls makeKeyWindow() cannot retarget this dispatch
    // midway when the caller passed the key-window member itself.
    std::shared_ptr<Window> window = target;
    if (!window) return;
    // Only windows registered with this application receive events.
    if (std::find(this->windows.begin(), this->windows.end(), window) == this->windows.end()) return;

    Event event;
    event.type = eventType;
    event.userInfo = userInfo;
    event.timestamp = focus_timer_get_time();

    // For touch events, transform native panel coordinates to logical coordinates.
    // Each case is the inverse of the Display's rendering rotation.
    if (eventType == FOCUS_EVENT_TOUCH_DOWN ||
        eventType == FOCUS_EVENT_TOUCH_MOVED ||
        eventType == FOCUS_EVENT_TOUCH_UP) {
        if (auto display = window->getDisplay().lock()) {
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

        // Update cursor position tracker before gesture recognition / view dispatch.
        Size ws = window->getContentRect().size;
        window->getCursorManager().handleTouchEvent(event, ws.width - 1, ws.height - 1);
    }

    if (window->touchEnabled) {
        bool navEvent = isFocusNavigationEvent(event.type);

        // Focus-navigation events go to the focused view while engaged.
        if (navEvent && window->isFocusEngaged()) {
            if (std::shared_ptr<View> focusedView = window->focusedView.lock()) {
                focusedView->handleEvent(event);
            }
            return;
        }

        // Give the window first chance to handle (or swallow) any event.
        // For touch events this allows the window to intercept before
        // gesture recognizers and hit-test dispatch run.
        if (window->handleEvent(event)) return;

        // Unconsumed focus-navigation events fall through to focus navigation.
        if (navEvent) {
            this->handleLatentNavigationEvent(event, window);
            return;
        }

        // Other non-touch events were already offered to the window above.
        if (event.type != FOCUS_EVENT_TOUCH_DOWN &&
            event.type != FOCUS_EVENT_TOUCH_MOVED &&
            event.type != FOCUS_EVENT_TOUCH_UP &&
            event.type != FOCUS_EVENT_LONG_PRESS) {
            return;
        }

        if (this->handleSystemGestures(event, window)) return;
        this->dispatchTouchEvent(event, window);
    } else if (std::shared_ptr<View> focusedView = window->focusedView.lock()) {
        focusedView->handleEvent(event);
    }
}

void Application::makeKeyWindow(const std::shared_ptr<Window>& window) {
    if (std::find(this->windows.begin(), this->windows.end(), window) != this->windows.end()) {
        this->window = window;
        // A d-pad window with nothing focused would silently drop non-positional
        // events, so focus it, as run() does for the constructor window.
        if (!window->touchEnabled && !window->focusedView.lock()) {
            window->becomeFocused();
        }
    }
}

std::shared_ptr<Window> Application::getKeyWindow() const {
    return this->window;
}

std::shared_ptr<Window> Application::getWindow() {
    return this->window;
}

void Application::setRootViewController(std::shared_ptr<ViewController> viewController) {
    this->window->setRootViewController(viewController);
}

bool Application::isModalPresented() const {
    return this->window->isModalPresented();
}

std::shared_ptr<ViewController> Application::activeViewController() const {
    return this->window->activeViewController();
}

void Application::presentViewController(std::shared_ptr<ViewController> viewController) {
    this->window->presentViewController(viewController);
}

void Application::dismissViewController() {
    this->window->dismissViewController();
}

void Application::dismissAllViewControllers() {
    this->window->dismissAllViewControllers();
}

void Application::quit() {
    this->running = false;
}

}  // namespace focus
