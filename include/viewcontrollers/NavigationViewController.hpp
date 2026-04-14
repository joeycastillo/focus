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

/**
 * @file NavigationViewController.hpp
 * @brief A container view controller that manages a stack of child view controllers.
 *
 * NavigationViewController presents one child view controller at a time with a
 * navigation bar showing the current title and a back button. Pushing a new VC
 * onto the stack destroys the previous VC's view to conserve memory; popping
 * recreates it via createView().
 *
 * The root view controller (provided at creation) cannot be popped.
 */

#pragma once

#include "ViewController.hpp"
#include <vector>
#include <memory>

class NavigationBar;

/**
 * @brief Container view controller with a push/pop navigation stack.
 *
 * Use the static create() factory method to construct. Present via
 * Application::setRootViewController() or Application::presentViewController().
 *
 * Child view controllers can access their NavigationViewController via
 * ViewController::getNavigationController().
 */
class NavigationViewController : public ViewController {
public:
    /**
     * @brief Create a navigation view controller with a root child.
     * @param application The owning application.
     * @param rootViewController The initial (bottom) view controller. Cannot be popped.
     * @return A shared_ptr to the NavigationViewController.
     */
    static std::shared_ptr<NavigationViewController> create(
        std::shared_ptr<Application> application,
        std::shared_ptr<ViewController> rootViewController);

    /**
     * @brief Push a view controller onto the navigation stack.
     *
     * The current top VC's view is destroyed. The new VC's view is created
     * and displayed. The navigation bar updates to show the new title and
     * a back button.
     *
     * @param viewController The view controller to push.
     */
    virtual void pushViewController(std::shared_ptr<ViewController> viewController);

    /**
     * @brief Pop the top view controller from the stack.
     *
     * Does nothing if only the root VC remains. The popped VC's view is
     * destroyed, and the new top VC's view is recreated.
     */
    virtual void popViewController();

    /// @brief Pop all view controllers above the root.
    /// The default implementation calls popViewController() in a loop.
    virtual void popToRootViewController();

    /// @brief Get the view controller currently on top of the stack.
    std::shared_ptr<ViewController> topViewController() const;

    /// @brief Get the number of view controllers on the stack.
    size_t stackDepth() const;

    /// @brief Set a right button on the navigation bar. Pass empty title to hide.
    void setRightButton(const std::string& title, std::function<void()> action);

    // ViewController lifecycle overrides
    void viewWillAppear() override;
    void viewDidLayoutSubviews() override;
    void viewDidAppear() override;
    void viewWillDisappear() override;
    void viewDidDisappear() override;

protected:
    NavigationViewController(std::shared_ptr<Application> application,
                             std::shared_ptr<ViewController> rootViewController);
    void createView() override;

    std::shared_ptr<NavigationBar> navigationBar;
    std::shared_ptr<View> contentArea;

    /// @brief Transition from one child VC to another, managing lifecycles and views.
    /// Override to customize how child view controllers are swapped in and out.
    virtual void transitionFromViewController(
        std::shared_ptr<ViewController> oldVC,
        std::shared_ptr<ViewController> newVC);

private:
    friend class ViewController;

    /// @brief Update the navigation bar title and back button visibility.
    void updateNavigationBar();

    std::vector<std::shared_ptr<ViewController>> viewControllerStack;
    std::string rightButtonTitle;
    std::function<void()> rightButtonAction;
};
