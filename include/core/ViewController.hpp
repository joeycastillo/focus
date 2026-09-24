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

/**
 * @file ViewController.hpp
 * @brief Controller that manages a view's lifecycle and coordinates with the Application.
 *
 * ViewController follows the UIKit pattern: each ViewController owns a single
 * root view that it creates lazily (on first appearance) and destroys when it
 * disappears. Subclass ViewController and override createView() to build your
 * view hierarchy.
 *
 * The Application manages ViewController presentation via setRootViewController(),
 * presentViewController(), and dismissViewController().
 */

#pragma once

#include "Focus.hpp"
#include <string>

namespace focus {

class NavigationViewController;
class TabViewController;

/**
 * @brief Abstract controller that manages a view and its lifecycle.
 *
 * The lifecycle follows a predictable sequence:
 * 1. viewWillAppear() — called before the view is added to the window.
 *    The base implementation calls createView() if the view doesn't exist yet.
 * 2. viewDidLayoutSubviews() — called after the container has finalized the
 *    root view's frame. Use this for size-dependent layout work.
 * 3. viewDidAppear() — called after the view is added to the window.
 * 4. viewWillDisappear() — called before the view is removed.
 * 5. viewDidDisappear() — called after the view is removed.
 *    The base implementation calls destroyView() to release the view.
 *
 * A controller disappears whenever it stops being visible: pushed over,
 * switched away from, replaced, or covered by an opaque modal. When it is
 * visible again it appears with a new view from createView(), so keep
 * anything that must survive in the controller.
 * @ingroup core
 */
class ViewController : public std::enable_shared_from_this<ViewController> {
public:
    /**
     * @brief Construct a view controller associated with an application.
     * @param application The owning application (stored as a weak reference).
     */
    ViewController(std::shared_ptr<Application> application);
    virtual ~ViewController() = default;

    /// @brief Called before the view is added to the window. Creates the view if needed.
    virtual void viewWillAppear();
    /// @brief Called after the container has finalized this view controller's root
    ///        view frame. Override to perform size-dependent layout work (e.g.
    ///        reloading a collection with the correct page dimensions).
    virtual void viewDidLayoutSubviews() {};
    /// @brief Called after the view has been added to the window.
    virtual void viewDidAppear() {};
    /// @brief Called before the view is removed from the window.
    virtual void viewWillDisappear() {};
    /// @brief Called after the view has been removed. Destroys the view by default.
    virtual void viewDidDisappear();

    /**
     * @brief Inject an event into the application's event dispatch system.
     *
     * Convenience method that forwards to Application::generateEvent().
     *
     * @param eventType One of the FOCUS_EVENT_* constants.
     * @param userInfo Event-specific payload (default 0).
     */
    void generateEvent(int32_t eventType, int32_t userInfo = 0);

    /// @brief Get the title for this view controller (displayed in navigation bars, etc.).
    std::string getTitle() const;

    /// @brief Set the title for this view controller.
    void setTitle(const std::string& title);

    /// @brief Get the navigation controller managing this view controller, if any.
    std::shared_ptr<NavigationViewController> getNavigationController() const;

    /// @brief Get the tab view controller managing this view controller, if any.
    std::shared_ptr<TabViewController> getTabViewController() const;

    /// @brief Get the root view managed by this controller, or nullptr if not yet created.
    std::shared_ptr<View> getView() const;

protected:
    /**
     * @brief Create this controller's view hierarchy.
     *
     * Subclasses must override this to build their view tree. Assign the
     * root of your view tree to this->view.
     */
    virtual void createView() = 0;

    /// @brief Destroy the view hierarchy, releasing the root view.
    virtual void destroyView();

    std::shared_ptr<View> view;            ///< The root view managed by this controller.
    std::weak_ptr<Application> application; ///< Weak reference to the owning application.
    std::string title;                     ///< Title displayed in navigation bars.
    std::weak_ptr<NavigationViewController> navigationController; ///< Set by NavigationViewController when pushed.
    std::weak_ptr<TabViewController> tabViewController; ///< Set by TabViewController when added as a tab.

public:
    /// @brief Stable programmatic identifier for this view controller.
    ///
    /// Used by test harnesses and accessibility tools to identify which
    /// screen is currently active (e.g. "home-screen", "settings").
    /// Set by application code; empty by default.
    std::string accessibilityIdentifier;

protected:
    friend class Application;
    friend class NavigationViewController;
    friend class TabViewController;
};

}  // namespace focus
