/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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

#include "View.hpp"
#include <string>
#include <functional>

class Font;
class CanvasView;

class KeyboardView : public View {
public:
    using KeyCallback = std::function<void(std::string key)>;

    KeyboardView(Rect rect);

    void setKeyCallback(KeyCallback callback);
    void setFont(std::shared_ptr<Font> font);

    void draw(int x, int y) override;
    bool handleEvent(Event event) override;

private:
    enum class KeyboardPage {
        Lowercase,
        Uppercase,
        Symbols
    };

    struct KeyRect {
        Rect rect;
        std::string label;
        std::string value; // what gets sent via callback
    };

    KeyCallback keyCallback;
    std::shared_ptr<Font> font;
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    KeyboardPage currentPage = KeyboardPage::Lowercase;

    void renderCanvas();
    void buildKeyLayout(std::vector<KeyRect>& keys) const;
    int getKeyForTouch(Point localPoint) const;

    // Cached key layout for hit testing
    mutable std::vector<KeyRect> cachedKeys;
    mutable bool keysCached = false;
};
