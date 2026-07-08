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
 * @file Focus.hpp
 * @brief Core header for the Focus UI framework.
 *
 * Focus is a lightweight, platform-agnostic UI framework for embedded devices.
 * It provides a view hierarchy with layout, event dispatch, focus management,
 * and touch input. This header defines the fundamental types, event constants,
 * and geometry primitives used throughout the framework.
 *
 * Include this header to get access to all core Focus types. Individual component
 * headers (View.hpp, Window.hpp, etc.) include this header automatically.
 */

#pragma once

#include "detail/stl_guard.h"

#include <stdint.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

#include "Utf8.hpp"  // UNICODE_CODEPOINT (aliased below as focus::CodePoint)

/// @name Directional Navigation Events
/// @brief D-pad or arrow-key navigation events for focus traversal.
/// @{
#define FOCUS_EVENT_DIRECTION_LEFT (1)
#define FOCUS_EVENT_DIRECTION_DOWN (2)
#define FOCUS_EVENT_DIRECTION_UP (3)
#define FOCUS_EVENT_DIRECTION_RIGHT (4)
/// @}

/// @name Action Events
/// @brief User action events (select, back/forward navigation, value changes).
/// @{
#define FOCUS_EVENT_SELECT (100)     ///< Confirm/activate the focused control.
#define FOCUS_EVENT_BACK (101)       ///< Navigate backward.
#define FOCUS_EVENT_FORWARD (102)    ///< Navigate forward.
#define FOCUS_EVENT_VALUE_CHANGED (103) ///< A control's value has changed (e.g. slider, checkbox).
/// @}

/// @name System Events
/// @{
#define FOCUS_EVENT_POWER_BUTTON (200) ///< Hardware power/lock button pressed.
/// @}

/// @name Accessibility Navigation Events
/// @brief Linear focus traversal for minimal input devices (rotary encoders,
/// switch access). Walks focusable views in depth-first document order,
/// wrapping at both ends.
/// @{
#define FOCUS_EVENT_ACCESSIBILITY_NEXT (300)     ///< Move focus to the next element.
#define FOCUS_EVENT_ACCESSIBILITY_PREVIOUS (301) ///< Move focus to the previous element.
/// @}

/// @name Touch Events
/// @brief Touch coordinates are packed into Event::userInfo as (x << 16 | y).
/// @{
#define FOCUS_EVENT_TOUCH_DOWN (1000)  ///< Finger touched the screen.
#define FOCUS_EVENT_TOUCH_MOVED (1001) ///< Finger moved while touching.
#define FOCUS_EVENT_TOUCH_UP (1002)    ///< Finger lifted from the screen.
#define FOCUS_EVENT_TOUCH_UP_INSIDE (1003)   ///< Finger lifted inside the captured view's bounds.
#define FOCUS_EVENT_TOUCH_UP_OUTSIDE (1004)  ///< Finger lifted outside the captured view's bounds.
#define FOCUS_EVENT_LONG_PRESS (1005)        ///< Finger held in place for ≥500ms without significant movement.
/// @}

/// @name Gesture Events
/// @brief Synthesized from touch sequences. Delivered to the captured view
/// instead of TOUCH_UP_INSIDE when the touch matches a gesture pattern.
/// userInfo encodes the touch-up coordinates as (x << 16 | y).
/// @{
#define FOCUS_EVENT_SWIPE_LEFT (1006)   ///< Quick horizontal swipe toward the left edge.
#define FOCUS_EVENT_SWIPE_RIGHT (1007)  ///< Quick horizontal swipe toward the right edge.
#define FOCUS_EVENT_SWIPE_UP (1008)     ///< Quick vertical swipe toward the top edge.
#define FOCUS_EVENT_SWIPE_DOWN (1009)   ///< Quick vertical swipe toward the bottom edge.
/// @}

/// @name Hardware Status Events
/// @{
#define FOCUS_EVENT_BUS_POWER_CHANGED (2000)    ///< External power connected or disconnected.
#define FOCUS_EVENT_CHARGE_STATE_CHANGED (2001) ///< Battery charge state changed.
#define FOCUS_EVENT_ORIENTATION_CHANGED (2002)  ///< Device orientation changed (userInfo is FocusOrientation).
#define FOCUS_EVENT_BATTERY_VOLTAGE_CHANGED (2003) ///< Battery voltage changed (userInfo is millivolts).
/// @}

/// @name Application-Defined Events
/// @{
/// Event codes 0-9999 are reserved for the Focus framework.
/// Application-defined events should start at FOCUS_EVENT_USER_BASE.
#define FOCUS_EVENT_USER_BASE (10000)
/// @}

namespace focus {

/// @brief A Unicode scalar value (code point).
///
/// uint32_t, or uint16_t when UNICODE_BMP_ONLY is defined. The text and glyph
/// APIs work in code points — character decoding, GlyphProvider lookup, and
/// text measurement all take a CodePoint.
/// @ingroup core
using CodePoint = UNICODE_CODEPOINT;

/// @brief Device orientation as detected by accelerometer.
enum class FocusOrientation {
    Unknown = 0,            ///< Orientation could not be determined.
    Portrait = 1,           ///< Device upright (Y+ up).
    PortraitUpsideDown = 2, ///< Device upside down (Y- up).
    LandscapeLeft = 3,      ///< Left edge down (X- up).
    LandscapeRight = 4,     ///< Right edge down (X+ up).
    FaceUp = 5,             ///< Screen facing up (Z+ up).
    FaceDown = 6,           ///< Screen facing down (Z- up).
};

class Application;
class Display;
class Window;
class View;
class Task;
class ViewController;

/// @brief An event delivered to views via the event dispatch system.
///
/// Events carry a type identifier, a userInfo payload, and a timestamp. The
/// meaning of userInfo is keyed by type: touch and gesture events pack the
/// coordinates as (x << 16) | y; FOCUS_EVENT_VALUE_CHANGED carries a
/// sender-specific value; other event types document their payload at the
/// FOCUS_EVENT_* definition. The timestamp records when the event was
/// generated, in microseconds (monotonic clock).
typedef struct {
    int32_t type;       ///< One of the FOCUS_EVENT_* constants.
    int32_t userInfo;   ///< Event-specific payload data.
    int64_t timestamp;  ///< Microseconds since boot. 0 if not set.
} Event;

/// @brief Callback type for event actions registered on views.
///
/// When a view receives an event matching a registered action type, it invokes
/// the corresponding Action with two arguments:
///
/// - **event** — the Event that triggered the action (type + userInfo).
/// - **sender** — a weak pointer to the view that fired the action. Lock it
///   to obtain a shared_ptr if you need to inspect or modify the sender
///   (e.g. read a control's value, disable a button after a tap). The pointer
///   is weak to avoid retain cycles between a view and its own closure.
typedef std::function<void(Event, std::weak_ptr<View>)> Action;

/// @brief A point in 2D space.
typedef struct {
    int x; ///< Horizontal coordinate.
    int y; ///< Vertical coordinate.
} Point;

/// @brief A 2D size with width and height.
typedef struct {
    int width;  ///< Width in pixels.
    int height; ///< Height in pixels.
} Size;

/// @brief A rectangle defined by an origin point and a size.
typedef struct {
    Point origin; ///< Top-left corner of the rectangle.
    Size size;    ///< Width and height of the rectangle.
} Rect;

/// @brief Construct a Point from x and y coordinates.
inline Point MakePoint(int x, int y) { return {x, y}; }
/// @brief Construct a Size from width and height.
inline Size MakeSize(int width, int height) { return {width, height}; }
/// @brief Construct a Rect from origin (x, y) and size (width, height).
inline Rect MakeRect(int x, int y, int width, int height) { return {{x, y}, {width, height}}; }

/// @brief A point at the origin (0, 0).
inline constexpr Point PointZero = {0, 0};
/// @brief A zero-sized size (0, 0).
inline constexpr Size SizeZero = {0, 0};
/// @brief A zero rect at the origin with no size.
inline constexpr Rect RectZero = {{0, 0}, {0, 0}};

/// @brief Test whether two Points are equal.
inline bool PointsEqual(Point a, Point b) { return (a.x == b.x) && (a.y == b.y); }
/// @brief Test whether two Sizes are equal.
inline bool SizesEqual(Size a, Size b) { return (a.width == b.width) && (a.height == b.height); }
/// @brief Test whether two Rects are equal.
inline bool RectsEqual(Rect a, Rect b) { return PointsEqual(a.origin, b.origin) && SizesEqual(a.size, b.size); }
/// @brief Test whether two Rects overlap.
inline bool RectsIntersect(Rect a, Rect b) {
    return a.origin.x < b.origin.x + b.size.width &&
           a.origin.x + a.size.width > b.origin.x &&
           a.origin.y < b.origin.y + b.size.height &&
           a.origin.y + a.size.height > b.origin.y;
}
/// @brief Test whether `outer` fully contains `inner`.
inline bool RectContains(Rect outer, Rect inner) {
    return outer.origin.x <= inner.origin.x &&
           outer.origin.y <= inner.origin.y &&
           outer.origin.x + outer.size.width >= inner.origin.x + inner.size.width &&
           outer.origin.y + outer.size.height >= inner.origin.y + inner.size.height;
}
/// @brief Compute the intersection of two Rects. Returns RectZero when they do not overlap.
inline Rect RectIntersection(Rect a, Rect b) {
    int x0 = a.origin.x > b.origin.x ? a.origin.x : b.origin.x;
    int y0 = a.origin.y > b.origin.y ? a.origin.y : b.origin.y;
    int ax1 = a.origin.x + a.size.width, bx1 = b.origin.x + b.size.width;
    int ay1 = a.origin.y + a.size.height, by1 = b.origin.y + b.size.height;
    int x1 = ax1 < bx1 ? ax1 : bx1;
    int y1 = ay1 < by1 ? ay1 : by1;
    if (x1 <= x0 || y1 <= y0) return RectZero;
    return MakeRect(x0, y0, x1 - x0, y1 - y0);
}

/// @brief Controls horizontal text alignment within a layout rect.
enum class TextAlignment {
    Left,      ///< Align text to the left edge (default).
    Center,    ///< Center text horizontally.
    Right,     ///< Align text to the right edge.
    Justified, ///< Distribute words evenly across the full width.
};

/// @brief Controls how a view's subviews are navigated with directional events.
///
/// When a view receives directional navigation events and the focused view is one
/// of its children, it uses the affinity to decide which direction maps to
/// "previous sibling" vs. "next sibling."
enum class DirectionalAffinity {
    None,       ///< View handles its own navigation; no automatic sibling nav.
    Vertical,   ///< Up/Down navigate between siblings.
    Horizontal, ///< Left/Right navigate between siblings.
};

/// @brief Type of on-screen keyboard to present for text input.
enum class KeyboardType {
    Default,    ///< Standard QWERTY keyboard with letters and symbols.
    NumberPad,  ///< Numeric keypad for integer input.
    DecimalPad, ///< Numeric keypad with decimal point.
};

/// @brief Semantic role of a UI element for accessibility consumers.
///
/// Accessibility roles describe what kind of element a view represents,
/// allowing screen readers, test harnesses, and other assistive tools to
/// understand how to interact with it. Views return their role from
/// accessibilityRole(). The default is None (not a meaningful element).
enum class AccessibilityRole {
    None,           ///< Structural container or decorative view; skip in accessibility tree.
    Button,         ///< A tappable button control.
    StaticText,     ///< Read-only text (e.g. LabelView).
    TextField,      ///< Editable text input.
    Image,          ///< An image or bitmap.
    Checkbox,       ///< A toggleable checkbox control.
    Slider,         ///< An adjustable slider control.
    ListItem,       ///< An item in a list (e.g. CollectionViewCell).
    List,           ///< A scrollable list of items (e.g. CollectionView).
    Header,         ///< A heading or section title.
    Tab,            ///< A tab in a tab bar.
    ProgressBar,    ///< A progress indicator.
    Adjustable,     ///< A value that can be incremented or decremented.
};

}  // namespace focus

