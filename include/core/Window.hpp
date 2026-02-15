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
 * An application typically has one Window, created by passing a Display and
 * a size. ViewControllers add their views as children of the window via the
 * Application's setRootViewController() and presentViewController() methods.
 */

#pragma once

#include "Focus.hpp"
#include "View.hpp"

class KeyboardView;

/**
 * @brief Root view that connects the view hierarchy to a Display.
 *
 * Window tracks dirty regions and the focused view. When touch is enabled,
 * the window itself remains the focused view and touch events are dispatched
 * via hit-testing rather than focus traversal.
 */
class Window : public View {
public:
    /**
     * @brief Construct a window with a display backend and screen dimensions.
     * @param display The display to render to.
     * @param size The window dimensions in pixels.
     */
    Window(std::shared_ptr<Display>, Size size);

    /**
     * @brief Add a subview and auto-focus it if nothing is focused yet.
     * @param view The view to add as a child of the window.
     */
    void addSubview(std::shared_ptr<View> view) override;

    /// @brief Windows can always become focused (returns true).
    bool canBecomeFocused() override;

    /**
     * @brief Enable touch input mode.
     *
     * In touch mode, the window itself is always the focused view, and events
     * are dispatched to views via hit-testing rather than focus navigation.
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

    /// @brief Check whether any region needs to be redrawn.
    bool needsDisplay();

    /**
     * @brief Mark the entire window as needing (or not needing) a redraw.
     * @param needsDisplay If true, the entire window is marked dirty.
     *                     If false, the dirty flag is cleared.
     */
    void setNeedsDisplay(bool needsDisplay);

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

    /// @brief Called after focus changes. Presents or dismisses the keyboard
    /// depending on whether the newly focused view wants keyboard input.
    void onFocusedViewChanged();

    /// @brief Check whether a view is the keyboard or a descendant of it.
    bool isKeyboardView(std::shared_ptr<View> view);

    /// @brief Returns a weak_ptr to this window.
    std::weak_ptr<Window> getWindow() override;

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

protected:
    std::shared_ptr<Display> display;          ///< The display backend.
    std::weak_ptr<Application> application;    ///< Owning application (set by Application::run()).
    std::weak_ptr<View> focusedView;           ///< The currently focused view.
    bool dirty;                                ///< Whether any region needs redrawing.
    Rect dirtyRect;                            ///< Accumulated region that needs redrawing.
    bool touchEnabled;                         ///< Whether touch input mode is active.
    std::weak_ptr<View> capturedTouchView;     ///< View capturing current touch sequence.
    Point touchDownPoint;                      ///< Initial touch-down point in window coordinates.
    std::shared_ptr<KeyboardView> keyboard;    ///< Window-managed on-screen keyboard, or nullptr.
    KeyboardType currentKeyboardType = KeyboardTypeDefault; ///< Type of currently presented keyboard.

    void presentKeyboard();
    void dismissKeyboard();
    void onKeyPressed(std::string key);

    friend class Application;
    friend class View;
    friend class ViewController;
};
