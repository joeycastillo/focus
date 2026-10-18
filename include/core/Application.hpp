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
 * @file Application.hpp
 * @brief Central coordinator for the Focus UI application lifecycle.
 *
 * Application manages the main run loop, its windows, and a set of
 * background tasks. Subclass Application to implement setup() with your
 * app-specific initialization, then call run() to enter the main loop.
 *
 * The run loop repeatedly iterates through all registered Tasks, giving each
 * a chance to execute. Tasks that return true from run() are automatically
 * removed (one-shot tasks). The loop runs indefinitely.
 */

#pragma once

#include "detail/stl_guard.h"

#include <atomic>
#include "Focus.hpp"
#include "Window.hpp"

namespace focus {

/**
 * @brief Abstract base class for Focus UI applications.
 *
 * an Application owns one or more Windows and a set of cooperatively scheduled
 * Tasks. Each Window owns its own root view controller and modal stack; the
 * presentation methods here forward to the key window.
 * @ingroup core
 */
class Application : public std::enable_shared_from_this<Application> {
public:
    /**
     * @brief Construct an application with the given window.
     * @param window The first window, which starts as the key window.
     */
    Application(const std::shared_ptr<Window>& window);
    virtual ~Application() = default;

    /**
     * @brief Application-specific initialization.
     *
     * Called once at the start of run(). Subclasses must override this to
     * set the root view controller, add tasks, etc.
     */
    virtual void setup() = 0;

    /**
     * @brief Add a task to the cooperative run loop.
     *
     * The task's run() method is called once per loop iteration. If the
     * run() method returns true, the task is removed from the list.
     *
     * Must be called on the main thread.
     *
     * @param task The task to add.
     */
    void addTask(std::shared_ptr<Task> task);

    /**
     * @brief Enter the main run loop.
     *
     * Calls setup(), then repeatedly runs every registered task. Returns when
     * quit() is called, or when the last task removes itself and none remain.
     */
    void run();

    /**
     * @brief Inject an event into the key window's event dispatch.
     *
     * In touch-enabled mode, touch events are dispatched via hit-testing to
     * the deepest view under the touch point. Non-touch events are delivered
     * to the focused view. In non-touch mode, all events go to the focused view,
     * and propogate up the responder chain until a view responds. If no view
     * in the chain responds to an event, the Window will receive the event.
     *
     * @param eventType One of the FOCUS_EVENT_* constants.
     * @param userInfo Event-specific payload. For touch events, encodes
     *                 coordinates as (x << 16 | y).
     */
    void generateEvent(int32_t eventType, int32_t userInfo);

    /**
     * @brief Inject an event into a specific window.
     *
     * Dispatches exactly as the two-argument form does, but to `window`
     * instead of the key window. Use it for positional (touch) events from
     * a display other than the key window's; each window tracks its own
     * touch sequence and gesture recognition. The window must be one
     * registered with the application (the constructor's window or one
     * passed to addWindow()); a null window is ignored.
     *
     * @param eventType One of the FOCUS_EVENT_* constants.
     * @param userInfo Event-specific payload. For touch events, encodes
     *                 coordinates as (x << 16 | y).
     * @param window The window to dispatch the event in.
     */
    void generateEvent(int32_t eventType, int32_t userInfo, const std::shared_ptr<Window>& window);

    /**
     * @brief Make a registered window the key window.
     *
     * The key window receives the events from the two-argument
     * generateEvent(), including buttons and keys, and the presentation
     * methods on Application forward to it. Ignored unless `window` was
     * registered (the constructor's window, or one passed to addWindow()).
     *
     * @param window The window to make key.
     */
    void makeKeyWindow(const std::shared_ptr<Window>& window);

    /// @brief Get the key window: the constructor's window until `makeKeyWindow` is called.
    std::shared_ptr<Window> getKeyWindow() const;

    /// @brief Get the key window; returns the same window as getKeyWindow().
    std::shared_ptr<Window> getWindow();

    /**
     * @brief Register an additional window, one per display.
     *
     * The window's application pointer is bound immediately when the
     * application is already managed by a std::shared_ptr, otherwise when
     * run() starts.
     *
     * @param window The window to register.
     */
    void addWindow(std::shared_ptr<Window> window);

    /// @brief Get every registered window; the constructor's window is first.
    const std::vector<std::shared_ptr<Window>>& getWindows() const { return windows; }

    /**
     * @brief Set the key window's root view controller, replacing any existing one.
     *
     * Forwards to Window::setRootViewController(), which describes how
     * presented modals and the old root are removed.
     *
     * @param viewController The new root view controller.
     */
    virtual void setRootViewController(std::shared_ptr<ViewController> viewController);

    /// @brief Check whether any modal view controller is presented in the key window.
    bool isModalPresented() const;

    /// @brief Get the topmost active view controller in the key window.
    /// @see Window::activeViewController()
    std::shared_ptr<ViewController> activeViewController() const;

    /**
     * @brief Present a view controller modally in the key window.
     *
     * Forwards to Window::presentViewController(). To present in another
     * window, call presentViewController() on that window, or on a view
     * controller hosted by it. Calls to ViewController::presentViewController()
     * go straight to the controller's own window, so an override of this
     * method is not called on that path.
     *
     * @param viewController The view controller to present modally.
     */
    virtual void presentViewController(std::shared_ptr<ViewController> viewController);

    /// @brief Dismiss the topmost modal view controller in the key window.
    ///
    /// Forwards to Window::dismissViewController(). Built-in controllers such
    /// as AlertViewController dismiss through their own window via
    /// ViewController::dismissViewController(), so an override of this
    /// method is not called on those paths.
    virtual void dismissViewController();

    /// @brief Dismiss all modal view controllers in the key window.
    /// @see Window::dismissAllViewControllers()
    void dismissAllViewControllers();

    /**
     * @brief Get the number of completed run-loop iterations.
     *
     * Incremented once per full pass through all registered tasks.
     * Useful for external monitoring (e.g. a heartbeat watchdog) to
     * detect whether the cooperative loop is still making progress.
     * Safe to read from any thread.
     */
    uint32_t loopCount() const { return loopCounter_.load(std::memory_order_relaxed); }

    /**
     * @brief Check whether any registered task reports itself busy.
     *
     * True if any task's isBusy() returns true. Safe to call from within a
     * task's run() method.
     */
    bool anyTaskBusy() const;

    /**
     * @brief Request the application to stop its run loop.
     *
     * The run loop will exit after the current iteration completes.
     * Calling this from a task or event handler causes run() to return.
     */
    void quit();

protected:
    bool running = true;                                ///< Whether the run loop should continue.
    std::vector<std::shared_ptr<Task>> tasks;      ///< Registered background tasks.
    std::shared_ptr<Window> window;                ///< The key window; the constructor's window until makeKeyWindow() is called.
    std::vector<std::shared_ptr<Window>> windows;  ///< Every registered window; the constructor's window is first.

    /// @brief Detect whether a completed touch sequence was a swipe gesture.
    /// @param dx Horizontal displacement (positive = rightward).
    /// @param dy Vertical displacement (positive = downward).
    /// @param durationUs Duration of the touch in microseconds.
    /// @return A FOCUS_EVENT_SWIPE_* constant, or 0 if not a swipe.
    virtual int32_t detectSwipe(int dx, int dy, int64_t durationUs);

    /// @brief Dispatch a touch event in `window` through normal view hit-test and capture.
    virtual void dispatchTouchEvent(Event event, const std::shared_ptr<Window>& window);

    /// @brief True for the event types that drive focus navigation.
    static bool isFocusNavigationEvent(int32_t type);
    /// @brief Handle an unconsumed focus-navigation event while nothing is engaged in `window`.
    void handleLatentNavigationEvent(Event event, const std::shared_ptr<Window>& window);

    /// @brief Attempt to route a touch event through `window`'s system gesture recognizers.
    /// @return true if the event was consumed (buffered or dispatched to a recognizer).
    virtual bool handleSystemGestures(Event event, const std::shared_ptr<Window>& window);

    std::atomic<uint32_t> loopCounter_{0}; ///< Completed run-loop iterations (readable from any thread).
};

}  // namespace focus
