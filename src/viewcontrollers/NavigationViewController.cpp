/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
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

#include "NavigationViewController.hpp"
#include "NavigationBar.hpp"
#include "Application.hpp"
#include "Window.hpp"
#include <typeinfo>
#include "FocusLog.hpp"

namespace focus {

static const char *TAG = "Focus";

std::shared_ptr<NavigationViewController> NavigationViewController::create(
    std::shared_ptr<Application> application,
    std::shared_ptr<ViewController> rootViewController)
{
    auto navVC = std::shared_ptr<NavigationViewController>(
        new NavigationViewController(application, rootViewController));
    return navVC;
}

NavigationViewController::NavigationViewController(
    std::shared_ptr<Application> application,
    std::shared_ptr<ViewController> rootViewController)
    : ViewController(application)
{
    this->viewControllerStack.push_back(rootViewController);
}

void NavigationViewController::createView() {
    auto app = this->application.lock();
    if (!app) return;

    Size windowSize = app->getWindow()->getContentRect().size;

    // Full-screen container
    this->view = std::make_shared<View>(MakeRect(0, 0, windowSize.width, windowSize.height));

    // Navigation bar at top
    this->navigationBar = NavigationBar::create(windowSize.width);
    this->navigationBar->setBackAction([this]() {
        this->popViewController();
    });
    if (!this->rightButtonTitle.empty()) {
        this->navigationBar->setRightButton(this->rightButtonTitle, this->rightButtonAction);
    }
    this->view->addSubview(this->navigationBar);

    // Content area below the nav bar
    int navBarHeight = NavigationBar::getHeight();
    this->contentArea = std::make_shared<View>(
        MakeRect(0, navBarHeight, windowSize.width, windowSize.height - navBarHeight));
    this->view->addSubview(this->contentArea);

    // Handle FOCUS_EVENT_BACK to pop the stack
    this->view->setAction(
        [this](Event, std::weak_ptr<View>) {
            this->popViewController();
        },
        FOCUS_EVENT_BACK,
        this->weak_from_this());
}

void NavigationViewController::viewWillAppear() {
    // Create our own container view
    ViewController::viewWillAppear();

    // Show the top child VC's view inside the content area
    if (!this->viewControllerStack.empty() && this->contentArea) {
        auto topVC = this->viewControllerStack.back();
        topVC->navigationController = std::dynamic_pointer_cast<NavigationViewController>(
            this->shared_from_this());
        topVC->viewWillAppear();
        if (topVC->view) {
            topVC->view->setFrame(MakeRect(0, 0,
                this->contentArea->getFrame().size.width,
                this->contentArea->getFrame().size.height));
            topVC->viewDidLayoutSubviews();
            this->contentArea->addSubview(topVC->view);
        }
        this->updateNavigationBar();
    }
}

void NavigationViewController::viewDidLayoutSubviews() {
    if (!this->viewControllerStack.empty()) {
        this->viewControllerStack.back()->viewDidLayoutSubviews();
    }
}

void NavigationViewController::viewDidAppear() {
    if (!this->viewControllerStack.empty()) {
        this->viewControllerStack.back()->viewDidAppear();
    }
}

void NavigationViewController::viewWillDisappear() {
    if (!this->viewControllerStack.empty()) {
        this->viewControllerStack.back()->viewWillDisappear();
    }
}

void NavigationViewController::viewDidDisappear() {
    if (!this->viewControllerStack.empty()) {
        auto topVC = this->viewControllerStack.back();
        if (topVC->view && this->contentArea) {
            this->contentArea->removeSubview(topVC->view);
        }
        topVC->viewDidDisappear();
    }
    this->navigationBar.reset();
    this->contentArea.reset();
    ViewController::viewDidDisappear();
}

void NavigationViewController::pushViewController(std::shared_ptr<ViewController> viewController) {
    if (!this->contentArea) return;
    if (this->inTransition) return;
    FOCUS_LOGD(TAG, "push %s", typeid(*viewController).name());

    this->inTransition = true;
    auto oldVC = this->viewControllerStack.back();
    viewController->navigationController = std::dynamic_pointer_cast<NavigationViewController>(
        this->shared_from_this());
    this->viewControllerStack.push_back(viewController);
    this->transitionFromViewController(oldVC, viewController);
    this->updateNavigationBar();
    this->focusTopViewController();
    this->inTransition = false;
}

void NavigationViewController::popViewController() {
    if (this->viewControllerStack.size() <= 1) return;
    if (!this->contentArea) return;
    if (this->inTransition) return;

    this->inTransition = true;
    auto oldVC = this->viewControllerStack.back();
    FOCUS_LOGD(TAG, "pop %s", typeid(*oldVC).name());
    oldVC->navigationController.reset();
    this->viewControllerStack.pop_back();
    auto newVC = this->viewControllerStack.back();
    this->transitionFromViewController(oldVC, newVC);
    this->updateNavigationBar();
    this->focusTopViewController();
    this->inTransition = false;
}

void NavigationViewController::popToRootViewController() {
    while (this->viewControllerStack.size() > 1) {
        this->popViewController();
    }
}

std::shared_ptr<ViewController> NavigationViewController::topViewController() const {
    if (this->viewControllerStack.empty()) return nullptr;
    return this->viewControllerStack.back();
}

size_t NavigationViewController::stackDepth() const {
    return this->viewControllerStack.size();
}

void NavigationViewController::setRightButton(const std::string& title, std::function<void()> action) {
    this->rightButtonTitle = title;
    this->rightButtonAction = action;
    if (this->navigationBar) {
        this->navigationBar->setRightButton(title, action);
    }
}

void NavigationViewController::transitionFromViewController(
    std::shared_ptr<ViewController> oldVC,
    std::shared_ptr<ViewController> newVC)
{
    // Tear down the old VC's view
    if (oldVC && oldVC->view) {
        oldVC->viewWillDisappear();
        this->contentArea->removeSubview(oldVC->view);
        oldVC->viewDidDisappear(); // destroys the old view
    }

    // Set up the new VC's view
    newVC->viewWillAppear(); // creates the view via createView()
    if (newVC->view) {
        newVC->view->setFrame(MakeRect(0, 0,
            this->contentArea->getFrame().size.width,
            this->contentArea->getFrame().size.height));
        newVC->viewDidLayoutSubviews();
        this->contentArea->addSubview(newVC->view);
    }
    newVC->viewDidAppear();
}

void NavigationViewController::focusTopViewController() {
    if (!this->view) return;
    auto window = this->view->getWindow().lock();
    if (!window || window->isTouchEnabled()) return;

    auto topVC = this->topViewController();
    std::shared_ptr<View> target =
        (topVC && topVC->view) ? topVC->view->firstFocusableDescendant() : nullptr;
    if (!target && this->navigationBar) {
        target = this->navigationBar->firstFocusableDescendant();
    }
    if (target) {
        target->becomeFocused();
    }
}

void NavigationViewController::updateNavigationBar() {
    if (!this->navigationBar) return;

    auto topVC = this->topViewController();
    if (topVC) {
        this->navigationBar->setTitle(topVC->getTitle());
    }
    this->navigationBar->setBackButtonVisible(this->viewControllerStack.size() > 1);
}

}  // namespace focus
