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
 * @file View.hpp
 * @brief Base class for all visual elements in the Focus UI framework.
 *
 * View is the fundamental building block of the Focus UI. It manages a
 * rectangular region on screen, a hierarchy of child views, event dispatch,
 * focus management, and dirty-rect tracking for efficient redraws.
 *
 * Views use two coordinate rectangles:
 * - **frame**: the view's position and size in its superview's coordinate system.
 * - **bounds**: the view's own coordinate system (origin is usually 0,0 but can
 *   be offset for scrolling or panning).
 *
 * Subviews are positioned relative to their parent's bounds, so adjusting a
 * view's bounds origin moves its children.
 */

#pragma once

#include "Focus.hpp"

/**
 * @brief Base class for all visual elements in the Focus view hierarchy.
 *
 * Views form a tree rooted at a Window. Each view draws itself and its children,
 * handles events (touch, directional navigation, actions), and participates in
 * the focus system. Subclass View to create custom visual elements.
 */
class View : public std::enable_shared_from_this<View> {
public:
    /**
     * @brief Construct a view with the given frame rectangle.
     * @param rect The view's position and size in its superview's coordinate system.
     *             The bounds are automatically set to the same size with origin (0,0).
     */
    View(Rect rect);
    ~View();

    /**
     * @brief Draw this view and its subviews.
     *
     * Called by the framework during the display refresh cycle. The x and y
     * parameters are the accumulated offset from the window origin — subclasses
     * should draw at (x + frame.origin.x, y + frame.origin.y).
     *
     * The base implementation fills the frame with backgroundColor if opaque,
     * then recursively draws all non-hidden subviews.
     *
     * @param x Horizontal offset from the window origin to the superview's content area.
     * @param y Vertical offset from the window origin to the superview's content area.
     */
    virtual void draw(int x, int y);

    /**
     * @brief Add a child view to this view's hierarchy.
     *
     * The subview's superview and window references are set automatically.
     * If attached to a window, adding a subview marks the display as dirty.
     *
     * @param view The view to add as a child.
     */
    virtual void addSubview(std::shared_ptr<View> view);

    /**
     * @brief Remove a child view from this view's hierarchy.
     *
     * If the removed view was focused, focus is transferred to the superview.
     * The display is marked as needing a redraw.
     *
     * @param view The child view to remove.
     */
    virtual void removeSubview(std::shared_ptr<View> view);

    /// @brief Check whether this view currently has focus.
    bool isFocused();

    /**
     * @brief Whether this view is capable of receiving focus.
     *
     * Returns false by default. Override in subclasses (e.g. controls) that
     * should participate in the focus system.
     */
    virtual bool canBecomeFocused();

    /**
     * @brief Attempt to make this view the focused view in its window.
     *
     * If canBecomeFocused() returns true, the previously focused view is
     * notified via the willResignFocus/didResignFocus callbacks, and this
     * view receives willBecomeFocused/didBecomeFocused.
     *
     * In touch-enabled windows, only the window itself can be focused.
     *
     * @return true if focus was successfully acquired.
     */
    virtual bool becomeFocused();

    /**
     * @brief Give up focus, passing it to the superview.
     *
     * Called automatically when a focused view is removed from its superview.
     */
    virtual void resignFocus();

    /// @brief Called when this view is added to a window's hierarchy.
    virtual void movedToWindow();

    /// @name Focus Lifecycle Callbacks
    /// @brief Override these to respond to focus changes (e.g. redraw highlights).
    /// @{
    virtual void willBecomeFocused();
    virtual void didBecomeFocused();
    virtual void willResignFocus();
    virtual void didResignFocus();
    /// @}

    /// @name Text Input
    /// @brief Override these in views that accept keyboard text input (e.g. TextField).
    /// @{

    /// @brief Whether this view accepts keyboard text input when focused.
    /// The Window uses this to auto-present an on-screen keyboard.
    virtual bool wantsKeyboardInput();

    /// @brief Insert text at the current input position.
    virtual void insertText(const std::string& text);

    /// @brief Delete the character before the current input position.
    virtual void deleteBackward();
    /// @}

    /**
     * @brief Handle an event delivered to this view.
     *
     * If an action is registered for the event type, the action callback is
     * invoked. Otherwise, directional events are used to navigate focus among
     * sibling subviews (respecting directionalAffinity). Unhandled events
     * bubble up to the superview.
     *
     * @param event The event to handle.
     * @return true if the event was consumed, false if it should continue bubbling.
     */
    virtual bool handleEvent(Event event);

    /**
     * @brief Register a callback for a specific event type.
     *
     * When this view receives an event with a matching type, the action is
     * called instead of the default event handling.
     *
     * @param action The callback to invoke.
     * @param type The event type to match (e.g. FOCUS_EVENT_SELECT).
     */
    void setAction(const Action &action, int32_t type);

    /**
     * @brief Remove a previously registered action for an event type.
     * @param type The event type whose action should be removed.
     */
    void removeAction(int32_t type);

    /// @brief Get this view's parent view, or an empty weak_ptr if none.
    virtual std::weak_ptr<View>getSuperview();

    /// @brief Get the window this view belongs to, or an empty weak_ptr if detached.
    virtual std::weak_ptr<Window> getWindow();

    /**
     * @brief Set the window reference for this view and all its subviews.
     *
     * Called internally when views are added to a window's hierarchy.
     * @param window The window to associate with.
     */
    virtual void setWindow(std::shared_ptr<Window> window);

    /// @brief Get the view's frame (position and size in superview coordinates).
    Rect getFrame();

    /**
     * @brief Set the view's frame, updating bounds size to match.
     *
     * If attached to a window, marks the union of the old and new frame as dirty.
     * @param rect The new frame rectangle.
     */
    void setFrame(Rect rect);

    /// @brief Get the view's bounds (its own coordinate system, used for scrolling).
    Rect getBounds();

    /**
     * @brief Set the view's bounds rectangle.
     *
     * Changing bounds.origin scrolls the view's content. The frame on screen
     * does not move.
     * @param rect The new bounds rectangle.
     */
    void setBounds(Rect rect);

    /// @brief Whether this view fills its frame with its background color before drawing.
    bool isOpaque();
    /// @brief Set whether this view should fill its frame with backgroundColor.
    void setOpaque(bool value);

    /// @brief Whether this view is hidden (hidden views are not drawn).
    bool isHidden();
    /// @brief Set whether this view should be hidden.
    void setHidden(bool value);

    /// @brief Get the view's background fill color.
    uint16_t getBackgroundColor();
    /// @brief Set the view's background fill color.
    void setBackgroundColor(uint16_t value);

    /// @brief Get the view's foreground (text/border) color.
    uint16_t getForegroundColor();
    /// @brief Set the view's foreground (text/border) color.
    void setForegroundColor(uint16_t value);

    /// @brief Get the directional affinity for focus navigation among subviews.
    uint16_t getDirectionalAffinity();
    /// @brief Set the directional affinity (vertical or horizontal) for subview navigation.
    void setDirectionalAffinity(DirectionalAffinity value);

    /**
     * @brief Mark a region as needing redraw.
     *
     * The rect is converted to window coordinates and accumulated into the
     * window's dirty rect for the next refresh cycle.
     *
     * @param rect The region to invalidate, in this view's superview coordinates.
     */
    void setNeedsDisplayInRect(Rect rect);

    /**
     * @brief Find the deepest subview containing a touch point.
     *
     * Searches subviews in reverse order (front-to-back) so that views drawn
     * on top receive touch priority. Returns this view if no subview contains
     * the point.
     *
     * @param touch Touch point in this view's superview coordinate system.
     * @return Weak pointer to the view that should receive the touch event.
     */
    std::weak_ptr<View> getViewForTouch(Point touch);

    /// Converts a point from window coordinates to this view's local coordinate system,
    /// accounting for the full superview chain (frame origins and bounds offsets).
    Point convertPointFromWindow(Point windowPoint);

    /// @brief Test if a point (in window coordinates) is inside this view's bounds.
    bool containsPointInWindowCoordinates(Point windowPoint);

    /// @brief Get a human-readable description of this view (type, address, tag, frame).
    std::string description();

    /// @brief Clear the touch-checked flag on this view and all descendants.
    void clearTouchChecked();

    /// @brief Set the default background color for all newly created views.
    static void SetDefaultBackgroundColor(uint16_t color);
    /// @brief Set the default foreground color for all newly created views.
    static void SetDefaultForegroundColor(uint16_t color);

    int32_t tag = 0; ///< Application-defined tag for identifying views.

protected:
    /// Returns the display if this view is attached to a window, nullptr otherwise.
    /// Use this in draw() methods to safely get the display for rendering.
    std::shared_ptr<Display> getDisplayIfAttached();

    /// @brief Test whether a point (in superview coordinates) falls within this view's frame.
    bool _contains(Point point);
    bool _touch_checked = false; ///< Internal flag for touch hit-testing.

    bool focused = false;        ///< Whether this view currently has focus.
    bool opaque = true;          ///< Whether to fill the background before drawing.
    bool hidden = false;         ///< Whether this view is hidden from drawing.
    uint16_t backgroundColor;    ///< Background fill color.
    uint16_t foregroundColor;    ///< Foreground drawing color.
    Rect frame = {};             ///< Position and size in superview coordinates.
    Rect bounds = {};            ///< View's own coordinate system (origin usually 0,0).
    DirectionalAffinity affinity = DirectionalAffinityVertical; ///< Focus navigation direction.
    std::vector<std::shared_ptr<View>> subviews; ///< Child views, drawn in order (back to front).
    std::map<int32_t, Action> actions;           ///< Registered event action callbacks.
    std::weak_ptr<View> superview;               ///< Parent view in the hierarchy.

    static uint16_t defaultBackgroundColor; ///< Default background for new views.
    static uint16_t defaultForegroundColor; ///< Default foreground for new views.
private:
    std::weak_ptr<Window> window; ///< The window this view belongs to.

    friend class Window;
};

