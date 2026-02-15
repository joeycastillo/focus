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

#include <stdint.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

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

/// @name Hardware Status Events
/// @{
#define FOCUS_EVENT_BUS_POWER_CHANGED (2000)    ///< External power connected or disconnected.
#define FOCUS_EVENT_CHARGE_STATE_CHANGED (2001) ///< Battery charge state changed.
#define FOCUS_EVENT_ORIENTATION_CHANGED (2002)  ///< Device orientation changed (userInfo is FocusOrientation).
#define FOCUS_EVENT_BATTERY_VOLTAGE_CHANGED (2003) ///< Battery voltage changed (userInfo is millivolts).
#define FOCUS_EVENT_WIFI_CONNECTED (2004)        ///< WiFi connected (userInfo unused).
#define FOCUS_EVENT_WIFI_DISCONNECTED (2005)     ///< WiFi disconnected (userInfo unused).
#define FOCUS_EVENT_WIFI_SIGNAL_CHANGED (2006)   ///< WiFi signal level changed (userInfo = 0-3).
/// @}

/// @brief Device orientation as detected by accelerometer.
typedef enum {
    FOCUS_ORIENTATION_UNKNOWN = 0,            ///< Orientation could not be determined.
    FOCUS_ORIENTATION_PORTRAIT = 1,           ///< Device upright (Y+ up).
    FOCUS_ORIENTATION_PORTRAIT_UPSIDE_DOWN = 2, ///< Device upside down (Y- up).
    FOCUS_ORIENTATION_LANDSCAPE_LEFT = 3,     ///< Left edge down (X- up).
    FOCUS_ORIENTATION_LANDSCAPE_RIGHT = 4,    ///< Right edge down (X+ up).
    FOCUS_ORIENTATION_FACE_UP = 5,            ///< Screen facing up (Z+ up).
    FOCUS_ORIENTATION_FACE_DOWN = 6,          ///< Screen facing down (Z- up).
} FocusOrientation;

class Application;
class Display;
class Window;
class View;
class Task;
class ViewController;

/// @brief An event delivered to views via the event dispatch system.
///
/// Events carry a type identifier and an optional userInfo payload. For touch
/// events, userInfo encodes the touch coordinates as (x << 16 | y). For
/// FOCUS_EVENT_VALUE_CHANGED, userInfo carries a control-specific value.
typedef struct {
    int32_t type;     ///< One of the FOCUS_EVENT_* constants.
    int32_t userInfo; ///< Event-specific payload data.
} Event;

/// @brief Callback type for event actions registered on views.
///
/// When a view receives an event matching a registered action type, it invokes
/// the corresponding Action with the event and a weak reference to the sender.
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

/// @brief Controls horizontal text alignment within a layout rect.
typedef enum {
    TextAlignmentLeft,   ///< Align text to the left edge (default).
    TextAlignmentCenter, ///< Center text horizontally.
    TextAlignmentRight,  ///< Align text to the right edge.
} TextAlignment;

/// @brief Controls how a view's subviews are navigated with directional events.
///
/// When a view receives directional navigation events and the focused view is one
/// of its children, it uses the affinity to decide which direction maps to
/// "previous sibling" vs. "next sibling."
typedef enum {
    DirectionalAffinityNone,       ///< View handles its own navigation; no automatic sibling nav.
    DirectionalAffinityVertical,   ///< Up/Down navigate between siblings.
    DirectionalAffinityHorizontal, ///< Left/Right navigate between siblings.
} DirectionalAffinity;

/// @brief Type of on-screen keyboard to present for text input.
typedef enum {
    KeyboardTypeDefault,    ///< Standard QWERTY keyboard with letters and symbols.
    KeyboardTypeNumberPad,  ///< Numeric keypad for integer input.
    KeyboardTypeDecimalPad, ///< Numeric keypad with decimal point.
} KeyboardType;

#include "Color.hpp"
#include "NotificationCenter.hpp"
#include "UserSettings.hpp"
#include "Locale.hpp"
