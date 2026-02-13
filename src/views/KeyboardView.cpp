/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
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

#include "KeyboardView.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include <cstring>

KeyboardView::KeyboardView(Rect rect, KeyboardType type) : View(rect) {
    this->type = type;
}

void KeyboardView::setKeyCallback(KeyCallback callback) {
    this->keyCallback = callback;
}

void KeyboardView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->canvasValid = false;
    this->keysCached = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void KeyboardView::buildKeyLayout(std::vector<KeyRect>& keys) const {
    keys.clear();

    int w = this->frame.size.width;
    int h = this->frame.size.height;

    if (this->type == KeyboardTypeNumberPad || this->type == KeyboardTypeDecimalPad) {
        int rows = 4;
        int cols = 3;
        int rowHeight = h / rows;
        int keyWidth = w / cols;
        int keyPadding = 2;

        const char* digits[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
        for (int i = 0; i < 9; i++) {
            int row = i / 3;
            int col = i % 3;
            KeyRect kr;
            kr.rect = MakeRect(col * keyWidth + keyPadding, row * rowHeight + keyPadding,
                               keyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
            kr.label = digits[i];
            kr.value = digits[i];
            keys.push_back(kr);
        }

        // Bottom row: Del, 0, Done
        KeyRect delKey;
        delKey.rect = MakeRect(keyPadding, 3 * rowHeight + keyPadding,
                               keyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        delKey.label = "Del";
        delKey.value = "\b";
        keys.push_back(delKey);

        KeyRect zeroKey;
        zeroKey.rect = MakeRect(keyWidth + keyPadding, 3 * rowHeight + keyPadding,
                                keyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        zeroKey.label = "0";
        zeroKey.value = "0";
        keys.push_back(zeroKey);

        KeyRect doneKey;
        doneKey.rect = MakeRect(2 * keyWidth + keyPadding, 3 * rowHeight + keyPadding,
                                keyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        doneKey.label = "Done";
        doneKey.value = "\n";
        keys.push_back(doneKey);

        return;
    }

    // 4 rows of keys
    int rowCount = 4;
    int rowHeight = h / rowCount;
    int keyPadding = 2;

    // Define key labels for each page
    const char* row1Letters = nullptr;
    const char* row2Letters = nullptr;
    const char* row3Letters = nullptr;

    switch (this->currentPage) {
        case KeyboardPage::Lowercase:
            row1Letters = "qwertyuiop";
            row2Letters = "asdfghjkl";
            row3Letters = "zxcvbnm";
            break;
        case KeyboardPage::Uppercase:
            row1Letters = "QWERTYUIOP";
            row2Letters = "ASDFGHJKL";
            row3Letters = "ZXCVBNM";
            break;
        case KeyboardPage::Symbols:
            row1Letters = "1234567890";
            row2Letters = "!@#$%^&*(";
            row3Letters = ")-_=+.,";
            break;
    }

    // Row 1: 10 keys
    int row1Count = 10;
    int keyWidth1 = w / row1Count;
    for (int i = 0; i < row1Count; i++) {
        KeyRect kr;
        kr.rect = MakeRect(i * keyWidth1 + keyPadding, keyPadding,
                           keyWidth1 - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = std::string(1, row1Letters[i]);
        kr.value = kr.label;
        keys.push_back(kr);
    }

    // Row 2: 9 keys, centered with half-key offset
    int row2Count = (int)strlen(row2Letters);
    int keyWidth2 = w / 10; // same key width as row 1
    int row2Offset = (w - row2Count * keyWidth2) / 2;
    for (int i = 0; i < row2Count; i++) {
        KeyRect kr;
        kr.rect = MakeRect(row2Offset + i * keyWidth2 + keyPadding, rowHeight + keyPadding,
                           keyWidth2 - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = std::string(1, row2Letters[i]);
        kr.value = kr.label;
        keys.push_back(kr);
    }

    // Row 3: Shift/123 + 7 letters + Backspace
    int row3Count = (int)strlen(row3Letters);
    int specialKeyWidth = w * 3 / 20; // ~15% of width for shift/backspace
    int letterAreaWidth = w - 2 * specialKeyWidth;
    int keyWidth3 = letterAreaWidth / row3Count;
    int letterOffset = specialKeyWidth;

    // Shift or 123 key
    {
        KeyRect kr;
        kr.rect = MakeRect(keyPadding, 2 * rowHeight + keyPadding,
                           specialKeyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        if (this->currentPage == KeyboardPage::Symbols) {
            kr.label = "ABC";
            kr.value = "\x01"; // internal: switch to lowercase
        } else if (this->currentPage == KeyboardPage::Uppercase) {
            kr.label = "abc";
            kr.value = "\x01"; // internal: switch to lowercase
        } else {
            kr.label = "Shift";
            kr.value = "\x02"; // internal: switch to uppercase
        }
        keys.push_back(kr);
    }

    // Letter keys
    for (int i = 0; i < row3Count; i++) {
        KeyRect kr;
        kr.rect = MakeRect(letterOffset + i * keyWidth3 + keyPadding, 2 * rowHeight + keyPadding,
                           keyWidth3 - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = std::string(1, row3Letters[i]);
        kr.value = kr.label;
        keys.push_back(kr);
    }

    // Backspace key
    {
        KeyRect kr;
        kr.rect = MakeRect(w - specialKeyWidth + keyPadding, 2 * rowHeight + keyPadding,
                           specialKeyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = "Del";
        kr.value = "\b";
        keys.push_back(kr);
    }

    // Row 4: 123/ABC + Space + Done
    int symbolKeyWidth = w * 3 / 20;
    int doneKeyWidth = w * 3 / 20;
    int spaceWidth = w - symbolKeyWidth - doneKeyWidth;

    // 123/ABC toggle
    {
        KeyRect kr;
        kr.rect = MakeRect(keyPadding, 3 * rowHeight + keyPadding,
                           symbolKeyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        if (this->currentPage == KeyboardPage::Symbols) {
            kr.label = "ABC";
            kr.value = "\x01"; // switch to lowercase
        } else {
            kr.label = "123";
            kr.value = "\x03"; // switch to symbols
        }
        keys.push_back(kr);
    }

    // Space bar
    {
        KeyRect kr;
        kr.rect = MakeRect(symbolKeyWidth + keyPadding, 3 * rowHeight + keyPadding,
                           spaceWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = "Space";
        kr.value = " ";
        keys.push_back(kr);
    }

    // Done key
    {
        KeyRect kr;
        kr.rect = MakeRect(symbolKeyWidth + spaceWidth + keyPadding, 3 * rowHeight + keyPadding,
                           doneKeyWidth - 2 * keyPadding, rowHeight - 2 * keyPadding);
        kr.label = "Done";
        kr.value = "\n";
        keys.push_back(kr);
    }
}

void KeyboardView::renderCanvas() {
    if (!this->canvas) {
        this->canvas = std::make_shared<CanvasView>(
            MakeRect(0, 0, this->frame.size.width, this->frame.size.height));
    }

    std::shared_ptr<Font> resolvedFont = this->font ? this->font : Font::systemFont();
    GlyphProvider* providerPtr = nullptr;
    std::shared_ptr<GlyphProvider> glyphProvider;
    if (resolvedFont) {
        glyphProvider = resolvedFont->getSharedGlyphProvider();
        providerPtr = glyphProvider.get();
    }

    int lineHeight = providerPtr ? providerPtr->getGlyphRowCount() : 16;

    // Clear background
    this->canvas->clear(this->backgroundColor);

    if (resolvedFont) {
        this->canvas->setFont(resolvedFont);
    }

    // Build key layout
    this->buildKeyLayout(this->cachedKeys);
    this->keysCached = true;

    int blackColor = 0; // CanvasView: 0 = black

    for (const auto& key : this->cachedKeys) {
        // Draw key background (outlined rectangle)
        this->canvas->drawRect(key.rect.origin.x, key.rect.origin.y,
                               key.rect.size.width, key.rect.size.height, blackColor);

        // Draw key label centered
        if (providerPtr) {
            int textWidth = TextLayout::measureTextWidth(
                key.label.c_str(), 1, providerPtr);
            int textX = key.rect.origin.x + (key.rect.size.width - textWidth) / 2;
            if (textX < key.rect.origin.x) textX = key.rect.origin.x;
            int textY = key.rect.origin.y + (key.rect.size.height - lineHeight) / 2;
            Rect textRect = MakeRect(textX, textY,
                                     key.rect.size.width - (textX - key.rect.origin.x),
                                     lineHeight);
            this->canvas->drawText(textRect, blackColor, 1, key.label.c_str());
        }
    }

    this->canvasValid = true;
}

void KeyboardView::drawContent(int x, int y) {
    if (!this->canvasValid) this->renderCanvas();
    if (this->canvas) {
        if (std::shared_ptr<Display> display = this->getDisplayIfAttached()) {
            display->blitOpaque(x + this->frame.origin.x, y + this->frame.origin.y,
                                this->frame.size.width, this->frame.size.height,
                                this->canvas->getBufferData(), this->canvas->getRowBytes());
        }
    }
}

int KeyboardView::getKeyForTouch(Point localPoint) const {
    if (!this->keysCached) return -1;

    for (size_t i = 0; i < this->cachedKeys.size(); i++) {
        const auto& key = this->cachedKeys[i];
        if (localPoint.x >= key.rect.origin.x &&
            localPoint.x < key.rect.origin.x + key.rect.size.width &&
            localPoint.y >= key.rect.origin.y &&
            localPoint.y < key.rect.origin.y + key.rect.size.height) {
            return (int)i;
        }
    }
    return -1;
}

bool KeyboardView::handleEvent(Event event) {
    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        Point windowPoint = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
        Point localPoint = this->convertPointFromWindow(windowPoint);

        // Build keys if not cached
        if (!this->keysCached) {
            this->buildKeyLayout(this->cachedKeys);
            this->keysCached = true;
        }

        int keyIndex = this->getKeyForTouch(localPoint);
        if (keyIndex >= 0 && keyIndex < (int)this->cachedKeys.size()) {
            const std::string& value = this->cachedKeys[keyIndex].value;

            // Handle internal mode-switch keys
            if (value == "\x01") {
                // Switch to lowercase
                this->currentPage = KeyboardPage::Lowercase;
                this->canvasValid = false;
                this->keysCached = false;
                if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                    this->setNeedsDisplayInRect(this->frame);
                }
                return true;
            } else if (value == "\x02") {
                // Switch to uppercase
                this->currentPage = KeyboardPage::Uppercase;
                this->canvasValid = false;
                this->keysCached = false;
                if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                    this->setNeedsDisplayInRect(this->frame);
                }
                return true;
            } else if (value == "\x03") {
                // Switch to symbols
                this->currentPage = KeyboardPage::Symbols;
                this->canvasValid = false;
                this->keysCached = false;
                if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                    this->setNeedsDisplayInRect(this->frame);
                }
                return true;
            }

            // Fire callback for character keys
            if (this->keyCallback) {
                this->keyCallback(value);
            }

            // After typing a character in uppercase mode, switch back to lowercase
            if (this->currentPage == KeyboardPage::Uppercase &&
                value != "\b" && value != "\n" && value != " ") {
                this->currentPage = KeyboardPage::Lowercase;
                this->canvasValid = false;
                this->keysCached = false;
                if (std::shared_ptr<Window> window = this->getWindow().lock()) {
                    this->setNeedsDisplayInRect(this->frame);
                }
            }

            return true;
        }
    }

    return View::handleEvent(event);
}
