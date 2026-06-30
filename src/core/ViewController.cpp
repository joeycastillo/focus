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

#include "ViewController.hpp"
#include "NavigationViewController.hpp"
#include "TabViewController.hpp"
#include "Application.hpp"
#include "Window.hpp"

namespace focus {

ViewController::ViewController(std::shared_ptr<Application> application) {
    this->application = application;
}

void ViewController::viewWillAppear() {
    if (!this->view) {
        this->createView();
    }
}

void ViewController::viewDidDisappear() {
    this->destroyView();
}

void ViewController::generateEvent(int32_t eventType, int32_t userInfo) {
    if (!this->view) return;

    // unsure about this one: we generate an event and let it bubble up to the window,
    // where the application can listen for it. seems like wasted effort to get a message
    // from a view controller to the application.
    if (std::shared_ptr<Window> window = this->view->getWindow().lock()) {
        if (std::shared_ptr<Application> application = window->application.lock()) {
            application->generateEvent(eventType, userInfo);
        }
    }
}


void ViewController::destroyView() {
    this->view.reset();
}

std::string ViewController::getTitle() const {
    return this->title;
}

void ViewController::setTitle(const std::string& title) {
    this->title = title;
    if (auto navController = this->navigationController.lock()) {
        navController->updateNavigationBar();
    }
}

std::shared_ptr<NavigationViewController> ViewController::getNavigationController() const {
    return this->navigationController.lock();
}

std::shared_ptr<TabViewController> ViewController::getTabViewController() const {
    return this->tabViewController.lock();
}

std::shared_ptr<View> ViewController::getView() const {
    return this->view;
}

}  // namespace focus
