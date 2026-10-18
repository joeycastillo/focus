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
 * @file Window.hpp
 * @brief Root of the view hierarchy, bridging the Application and Display.
 *
 * Window is a specialized View that serves as the root of the entire view
 * hierarchy. It owns the Display reference and tracks which region of the
 * screen needs to be redrawn (dirty rect). It also manages the currently
 * focused view and supports touch or directional input modes.
 *
 * An application has one Window per Display, created by passing a Display and
 * a size. Each Window owns a root view controller and a stack of modally
 * presented view controllers, set via setRootViewController() and
 * presentViewController().
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"
#include "GestureRecognizer.hpp"
#include "CursorManager.hpp"

namespace focus {

class KeyboardView;
class ViewController;
class HatchedView;

/**
 * @brief Root view that connects the view hierarchy to a Display.
 *
 * Window tracks dirty regions and the focused view. When touch is enabled,
 * touch events are dispatched via hit-testing; the window is focused while
 * focus navigation is idle, and unconsumed focus-navigation events summon
 * and drive focus among focusable descendants.
 * @ingroup core
 */
class Window : public View {
public:
    /**
     * @brief Construct a window with a display backend and screen dimensions.
     * @param display The display to render to.
     * @param size The window dimensions in pixels.
     */
    Window(std::shared_ptr<Display> display, Size size);

    /**
     * @brief Add a subview and auto-focus it if nothing is focused yet.
     * @param view The view to add as a child of the window.
     */
    void addSubview(std::shared_ptr<View> view) override;

    /// @brief Windows can always become focused (returns true).
    bool canBecomeFocused() const override;

    /**
     * @brief Enable touch input mode.
     *
     * In touch mode, touch events are dispatched to views via hit-testing.
     * The window starts as the focused view; focus-navigation events
     * (direction, select, back, accessibility traversal) that the window
     * does not consume summon and drive focus, so a touch UI is fully
     * navigable from a D-pad or similar input with no additional app code.
     * A touch outside the focused view returns focus to the window.
     */
    void setTouchEnabled();

    /// @brief Check whether touch input is enabled.
    bool isTouchEnabled();

    /// @brief Get the view currently capturing touch events, or empty if none.
    std::weak_ptr<View> getCapturedTouchView();

    /// @brief Capture a view to receive all touch events until TOUCH_UP.
    void setCapturedTouchView(std::weak_ptr<View> view, Point touchDownPoint);

    /// @brief Clear the captured touch view (called after TOUCH_UP).
    void clearCapturedTouchView();

    /// @brief Get the initial touch-down point for the current touch sequence.
    Point getTouchDownPoint();

    /// @name System Gesture Recognizers
    /// @brief Gesture recognizers that intercept touches before view dispatch.
    /// @{

    /// @brief Add a system gesture recognizer to the window.
    void addSystemGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer);

    /// @brief Remove a system gesture recognizer from the window.
    void removeSystemGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer);

    /// @brief Get the list of active system gesture recognizers.
    const std::vector<std::shared_ptr<GestureRecognizer>>& getSystemGestureRecognizers() const;

    /// @}

    /// @brief Check whether any region needs to be redrawn.
    bool needsDisplay();

    /**
     * @brief Clear the dirty flag after the display has been updated.
     *
     * Called by the refresh task after drawing the dirty region to the display.
     * To mark a region as needing redraw, use setNeedsDisplayInRect() instead.
     */
    void clearNeedsDisplay();

    /// @brief Get the region that needs to be redrawn, or a zero-size rect if clean.
    Rect getDirtyRect();

    /**
     * @brief Accumulate a dirty rect.
     *
     * The given rect is unioned with the existing dirty region. This is called
     * by views when they invalidate themselves via View::setNeedsDisplayInRect().
     *
     * @param rect The newly invalidated region in window coordinates.
     */
    void setNeedsDisplayInRect(Rect rect);

    /// @brief Get a weak reference to the display backend.
    std::weak_ptr<Display> getDisplay();

    /// @brief Get the currently focused view, or empty if none.
    std::weak_ptr<View> getFocusedView();

    /// @brief Check whether a view other than the window itself is focused.
    ///
    /// In touch windows this distinguishes the latent state (window focused,
    /// no focus indication) from the engaged state (a control is focused and
    /// focus-navigation events are being delivered to it).
    bool isFocusEngaged();

    /// @brief Called after focus changes. Presents or dismisses the keyboard
    /// depending on whether the newly focused view wants keyboard input.
    void onFocusedViewChanged();

    /// @brief Check whether a view is the keyboard or a descendant of it.
    bool isKeyboardView(std::shared_ptr<View> view);

    /// @brief Returns a weak_ptr to this window.
    std::weak_ptr<Window> getWindow() const override;

    /**
     * @brief Get the usable content area of the window.
     *
     * Subclasses that reserve screen space (e.g. for a status bar) should
     * override this to return the remaining area. The default returns the
     * full window frame. View controllers should use this to size their
     * views so they fit within the available content area.
     *
     * @return A Rect whose size is the usable content area.
     */
    virtual Rect getContentRect();
    /// @brief No-op (the window does not belong to another window).
    void setWindow(std::shared_ptr<Window> window) override;

    /// @brief Returns the owning Application (set by Application::addWindow() or Application::run()).
    std::weak_ptr<Application> getApplication() const { return application; }

    /// @brief Returns the cursor position tracker for trackpad-style input.
    CursorManager &getCursorManager() { return cursorManager; }

    /**
     * @brief Set the root view controller, replacing any existing one.
     *
     * Removes the modals presented at the time of the call, topmost first,
     * without showing anything beneath them. Then the old root disappears
     * and the new one appears. A modal presented while they are removed
     * stays up, above the new root.
     *
     * @param viewController The new root view controller.
     */
    void setRootViewController(std::shared_ptr<ViewController> viewController);

    /// @brief Get the root view controller, or nullptr if none is set.
    std::shared_ptr<ViewController> getRootViewController() const;

    /// @brief Check whether any modal view controller is currently presented in this window.
    bool isModalPresented() const;

    /**
     * @brief Get the topmost active view controller in this window.
     *
     * Returns the modal VC on top if any modals are presented, otherwise
     * returns the top of the navigation stack if the root VC is a
     * NavigationViewController, otherwise returns the root VC itself.
     */
    std::shared_ptr<ViewController> activeViewController() const;

    /**
     * @brief Present a view controller modally on top of this window's content.
     *
     * If the modal's view is opaque, it covers the controller beneath it,
     * which disappears and releases its view until the modal is dismissed.
     * Otherwise a HatchedView dimmer is inserted behind the modal and the
     * controller beneath gets no callbacks. Multiple modals can be stacked.
     *
     * @param viewController The view controller to present modally.
     */
    void presentViewController(std::shared_ptr<ViewController> viewController);

    /**
     * @brief Dismiss the topmost modal view controller in this window.
     *
     * Removes the modal's view and dimmer and shows whatever it covered.
     * Focus returns to the view that was focused before the modal was
     * presented, or to the first focusable view on top if that view was
     * rebuilt.
     */
    void dismissViewController();

    /**
     * @brief Dismiss all modal view controllers in this window.
     *
     * Removes the modals presented at the time of the call, topmost first,
     * without showing any of them, then shows what is beneath. A modal
     * presented while they are removed stays up. Focus returns to the view
     * that was focused before the first modal was presented, or to the
     * first focusable view on top if that view was rebuilt.
     */
    void dismissAllViewControllers();

protected:
    std::shared_ptr<Display> display;          ///< The display backend.
    std::weak_ptr<Application> application;    ///< Owning application (set by Application::addWindow() or Application::run()).
    std::weak_ptr<View> focusedView;           ///< The currently focused view.
    bool dirty = false;                        ///< Whether any region needs redrawing.
    Rect dirtyRect = {{0,0},{0,0}};            ///< Accumulated region that needs redrawing.
    bool touchEnabled = false;                 ///< Whether touch input mode is active.
    std::weak_ptr<View> capturedTouchView;     ///< View capturing current touch sequence.
    Point touchDownPoint;                      ///< Initial touch-down point in window coordinates.
    std::shared_ptr<KeyboardView> keyboard;    ///< Window-managed on-screen keyboard, or nullptr.
    KeyboardType currentKeyboardType = KeyboardType::Default; ///< Type of currently presented keyboard.
    std::vector<std::shared_ptr<GestureRecognizer>> systemGestureRecognizers; ///< Registered system gesture recognizers.
    CursorManager cursorManager;               ///< Trackpad-style cursor position tracker.

    bool longPressFired = false; ///< Set when LONG_PRESS is delivered during a touch; forces UP_OUTSIDE on release.
    int64_t touchDownTimestamp = 0; ///< Timestamp of the current touch sequence's TOUCH_DOWN event.

    /// @name System gesture recognition state
    /// @{
    std::vector<std::shared_ptr<GestureRecognizer>> activeRecognizers;  ///< Recognizers interested in current touch.
    std::vector<Event> gestureEventBuffer;                              ///< Buffered events during Possible state.
    bool gestureRecognitionPending = false;                             ///< Whether we're in the buffering phase.
    std::shared_ptr<GestureRecognizer> recognizedGesture;              ///< The recognizer that won, or nullptr.
    /// @}

    std::shared_ptr<ViewController> rootViewController; ///< The primary view controller.

    /// @brief Entry in the modal presentation stack.
    struct ModalEntry {
        std::shared_ptr<ViewController> viewController; ///< The presented view controller.
        std::shared_ptr<HatchedView> dimmer;            ///< Overlay that dims content behind the modal.
        std::weak_ptr<View> previousFocusedView;        ///< Focus to restore on dismiss.
        bool hidden = false;                            ///< Covered by an opaque modal; its view is released.
    };
    std::vector<ModalEntry> modalStack; ///< Stack of modally presented view controllers.

    bool rootHidden = false;        ///< The root is covered by an opaque modal.
    bool updatingCoverage = false;  ///< Set while updateCoverage() runs.

    /// Hide whatever an opaque modal covers and show whatever nothing covers.
    void updateCoverage();
    void hideViewController(std::shared_ptr<ViewController> viewController);
    /// Show a controller's view directly below `above`, or on top if null; `clipsFocus` keeps focus inside it.
    void showViewController(std::shared_ptr<ViewController> viewController,
                            std::shared_ptr<View> above, bool clipsFocus);
    void removeModal(const ModalEntry& entry);
    void restoreFocus(std::weak_ptr<View> previousFocusedView, bool wasEngaged);
    /// @brief Focus the first (or last) focusable view in the active context.
    void summonFocus(bool fromEnd);

    /// @brief Present the on-screen keyboard for the currently focused view.
    virtual void presentKeyboard();
    /// @brief Dismiss the on-screen keyboard.
    virtual void dismissKeyboard();
    /// @brief Handle a key press from the on-screen keyboard.
    virtual void onKeyPressed(std::string key);

    friend class Application;
    friend class View;
    friend class ViewController;
};

}  // namespace focus
