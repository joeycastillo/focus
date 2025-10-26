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

#include "Focus.hpp"

class View : public std::enable_shared_from_this<View> {
public:
    View(Rect rect);
    ~View();
    virtual void draw(int x, int y);
    virtual void addSubview(std::shared_ptr<View> view);
    void removeSubview(std::shared_ptr<View> view);
    bool isFocused();
    virtual bool canBecomeFocused();
    virtual bool becomeFocused();
    virtual void resignFocus();
    virtual void movedToWindow();
    virtual void willBecomeFocused();
    virtual void didBecomeFocused();
    virtual void willResignFocus();
    virtual void didResignFocus();
    virtual bool handleEvent(Event event);
    void setAction(const Action &action, int32_t type);
    void removeAction(int32_t type);
    virtual std::weak_ptr<View>getSuperview();
    virtual std::weak_ptr<Window> getWindow();
    virtual void setWindow(std::shared_ptr<Window> window);
    Rect getFrame();
    void setFrame(Rect rect);
    bool isOpaque();
    void setOpaque(bool value);
    bool isHidden();
    void setHidden(bool value);
    int32_t getTag();
    void setTag(int32_t value);
    uint16_t getBackgroundColor();
    void setBackgroundColor(uint16_t value);
    uint16_t getForegroundColor();
    void setForegroundColor(uint16_t value);
    uint16_t getDirectionalAffinity();
    void setDirectionalAffinity(DirectionalAffinity value);
    void setNeedsDisplayInRect(Rect rect);

    std::weak_ptr<View> getViewForTouch(Point touch);

    std::string description();

    // this whole touchChecked thing feels hacky!
    void clearTouchChecked();

    static void SetDefaultBackgroundColor(uint16_t color);
    static void SetDefaultForegroundColor(uint16_t color);
protected:
    bool _contains(Point point);
    bool _touch_checked = false;

    bool focused = false;
    bool opaque = true;
    bool hidden = false;
    int32_t tag = 0;
    uint16_t backgroundColor;
    uint16_t foregroundColor;
    Rect frame = {};
    DirectionalAffinity affinity = DirectionalAffinityVertical;
    std::vector<std::shared_ptr<View>> subviews;
    std::map<int32_t, Action> actions;
    std::weak_ptr<View> superview;

    static uint16_t defaultBackgroundColor;
    static uint16_t defaultForegroundColor;
private:
    std::weak_ptr<Window> window;

    friend class Window;
};

