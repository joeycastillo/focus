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
    this->window->becomeFocused();
    this->window->setNeedsDisplay(true);
    while(true) {
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
    if (this->window.get()->touchEnabled) {
        switch (event.type) {
            case FOCUS_EVENT_TOUCH_DOWN:
            case FOCUS_EVENT_TOUCH_MOVED:
            case FOCUS_EVENT_TOUCH_UP:
            {
                Point touch = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
                if (std::shared_ptr<View> touchedView = this->window.get()->getViewForTouch(touch).lock()) {
                    touchedView.get()->handleEvent(event);
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
