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

#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

#define FOCUS_EVENT_DIRECTION_LEFT (1)
#define FOCUS_EVENT_DIRECTION_DOWN (2)
#define FOCUS_EVENT_DIRECTION_UP (3)
#define FOCUS_EVENT_DIRECTION_RIGHT (4)
#define FOCUS_EVENT_SELECT (100)
#define FOCUS_EVENT_BACK (101)
#define FOCUS_EVENT_FORWARD (102)
#define FOCUS_EVENT_POWER_BUTTON (200)
#define FOCUS_EVENT_TOUCH_DOWN (1000)
#define FOCUS_EVENT_TOUCH_MOVED (1001)
#define FOCUS_EVENT_TOUCH_UP (1002)
#define FOCUS_EVENT_BUS_POWER_CHANGED (2000)
#define FOCUS_EVENT_CHARGE_STATE_CHANGED (2001)
#define FOCUS_EVENT_CARD_STATUS_CHANGED (2002)

class Application;
class Display;
class Window;
class View;
class Task;
class ViewController;

typedef struct {
    int32_t type;
    int32_t userInfo;
} Event;

typedef std::function<void(Event, std::weak_ptr<View>)> Action;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int width;
    int height;
} Size;

typedef struct {
    Point origin;
    Size size;
} Rect;

inline Point MakePoint(int x, int y) { return {x, y}; }
inline Size MakeSize(int width, int height) { return {width, height}; }
inline Rect MakeRect(int x, int y, int width, int height) { return {{x, y}, {width, height}}; }

inline bool PointsEqual(Point a, Point b) { return (a.x == b.x) && (a.y == b.y); }
inline bool SizesEqual(Size a, Size b) { return (a.width == b.width) && (a.height == b.height); }
inline bool RectsEqual(Rect a, Rect b) { return PointsEqual(a.origin, b.origin) && SizesEqual(a.size, b.size); }

typedef enum {
    DirectionalAffinityVertical,
    DirectionalAffinityHorizontal,
} DirectionalAffinity;
