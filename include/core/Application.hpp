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
 * Application manages the main run loop, the window, a stack of modal view
 * controllers, and a set of background tasks. Subclass Application to
 * implement setup() with your app-specific initialization, then call run()
 * to enter the main loop.
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

class HatchedView;

/**
 * @brief Abstract base class for Focus UI applications.
 *
 * an Application owns a Window and manages the rootViewController, a stack of
 * modally presented ViewControllers and a set of cooperatively scheduled Tasks.
 * @ingroup core
 */
class Application : public std::enable_shared_from_this<Application> {
public:
    /**
     * @brief Construct an application with the given window.
     * @param window The root window for this application's view hierarchy.
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
     * @brief Inject an event into the application's event dispatch system.
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

    /// @brief Get the application's root window.
    std::shared_ptr<Window> getWindow();

    /**
     * @brief Set the root view controller, replacing any existing one.
     *
     * The old root VC receives viewWillDisappear/viewDidDisappear callbacks
     * and its view is removed. The new root VC receives viewWillAppear/
     * viewDidAppear callbacks and its view is added to the window.
     *
     * @param viewController The new root view controller.
     */
    virtual void setRootViewController(std::shared_ptr<ViewController> viewController);

    /// @brief Check whether any modal view controller is currently presented.
    bool isModalPresented() const;

    /**
     * @brief Get the topmost active view controller.
     *
     * Returns the modal VC on top if any modals are presented, otherwise
     * returns the top of the navigation stack if the root VC is a
     * NavigationViewController, otherwise returns the root VC itself.
     */
    std::shared_ptr<ViewController> activeViewController() const;

    /**
     * @brief Present a view controller modally on top of the current content.
     *
     * A HatchedView dimmer is inserted behind the modal to visually dim the
     * content beneath. The previously focused view is saved and restored when
     * the modal is dismissed. Multiple modals can be stacked.
     *
     * @param viewController The view controller to present modally.
     */
    virtual void presentViewController(std::shared_ptr<ViewController> viewController);

    /**
     * @brief Dismiss the topmost modal view controller.
     *
     * Removes the modal's view and dimmer, restores focus to the previously
     * focused view, and marks the window as needing a full redraw.
     */
    virtual void dismissViewController();

    /**
     * @brief Dismiss all modal view controllers.
     *
     * Tears down every modal in the stack (topmost first), removing views
     * and dimmers. Focus is restored to the view that was focused before
     * the first modal was presented.
     */
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
    std::shared_ptr<Window> window;                ///< The root window.
    std::shared_ptr<ViewController> rootViewController; ///< The primary view controller.

    /// @brief Entry in the modal presentation stack.
    struct ModalEntry {
        std::shared_ptr<ViewController> viewController; ///< The presented view controller.
        std::shared_ptr<HatchedView> dimmer;            ///< Overlay that dims content behind the modal.
        std::weak_ptr<View> previousFocusedView;        ///< Focus to restore on dismiss.
    };
    std::vector<ModalEntry> modalStack; ///< Stack of modally presented view controllers.

    bool longPressFired = false; ///< Set when LONG_PRESS is delivered during a touch; forces UP_OUTSIDE on release.
    int64_t touchDownTimestamp = 0; ///< Timestamp of the current touch sequence's TOUCH_DOWN event.

    /// @brief Detect whether a completed touch sequence was a swipe gesture.
    /// @param dx Horizontal displacement (positive = rightward).
    /// @param dy Vertical displacement (positive = downward).
    /// @param durationUs Duration of the touch in microseconds.
    /// @return A FOCUS_EVENT_SWIPE_* constant, or 0 if not a swipe.
    virtual int32_t detectSwipe(int dx, int dy, int64_t durationUs);

    /// @brief Dispatch a touch event through normal view hit-test and capture.
    virtual void dispatchTouchEvent(Event event);

    /// @brief True for the event types that drive focus navigation.
    static bool isFocusNavigationEvent(int32_t type);
    /// @brief Handle an unconsumed focus-navigation event while nothing is engaged.
    void handleLatentNavigationEvent(Event event);
    /// @brief Focus the first (or last) focusable view in the active context.
    void summonFocus(bool fromEnd);

    /// @brief Attempt to route a touch event through system gesture recognizers.
    /// @return true if the event was consumed (buffered or dispatched to a recognizer).
    virtual bool handleSystemGestures(Event event);

    /// @name System gesture recognition state
    /// @{
    std::vector<std::shared_ptr<GestureRecognizer>> activeRecognizers;  ///< Recognizers interested in current touch.
    std::vector<Event> gestureEventBuffer;                              ///< Buffered events during Possible state.
    bool gestureRecognitionPending = false;                             ///< Whether we're in the buffering phase.
    std::shared_ptr<GestureRecognizer> recognizedGesture;              ///< The recognizer that won, or nullptr.
    /// @}

    std::atomic<uint32_t> loopCounter_{0}; ///< Completed run-loop iterations (readable from any thread).
};

}  // namespace focus
