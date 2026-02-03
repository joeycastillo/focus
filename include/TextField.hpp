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

#include "Control.hpp"
#include <string>
#include <functional>

class Font;
class CanvasView;

class TextField : public Control {
public:
    TextField(Rect rect);

    std::string getText() const;
    void setText(std::string text);
    void setPlaceholder(std::string placeholder);
    void setMaxLength(size_t maxLength);
    void setFont(std::shared_ptr<Font> font);
    std::shared_ptr<Font> getFont() const;
    void setTextChangedCallback(std::function<void(std::string)> callback);

    void draw(int x, int y) override;
    bool handleEvent(Event event) override;
    void didBecomeFocused() override;
    void didResignFocus() override;

    void insertText(const std::string& str);
    void deleteBackward();

protected:
    virtual std::string getDisplayText() const;

    std::string text;
    std::string placeholder;
    size_t maxLength = 0; // 0 = no limit
    std::shared_ptr<Font> font;
    std::function<void(std::string)> textChangedCallback;

private:
    std::shared_ptr<CanvasView> canvas;
    bool canvasValid = false;
    void renderCanvas();
};
