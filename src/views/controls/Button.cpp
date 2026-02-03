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

#include "Button.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "TextLayout.hpp"
#include "Font.hpp"

Button::Button(Rect rect, std::string text) : Control(rect) {
    this->text = text;
}

void Button::draw(int x, int y) {
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        View::draw(x, y);
        if (std::shared_ptr<Display> display = this->getWindow().lock()->getDisplay().lock()) {
            // Use button's font if set, otherwise fall back to display's default
            std::shared_ptr<GlyphProvider> glyphProvider;
            if (this->font) {
                glyphProvider = this->font->getSharedGlyphProvider();
            } else {
                glyphProvider = display->getDefaultGlyphProvider();
            }

            int lineHeight = 16;  // default fallback
            int textWidth = 0;
            if (glyphProvider) {
                lineHeight = glyphProvider->getGlyphRowCount();
                textWidth = TextLayout::measureTextWidth(this->text.c_str(), 1, glyphProvider.get());
            }

            // Count explicit newlines in the text
            int explicitNewlines = 0;
            for (char c : this->text) {
                if (c == '\n') explicitNewlines++;
            }

            int horizontalOffset = 0;
            int totalTextHeight = lineHeight;
            int layoutWidth = this->frame.size.width;

            if (explicitNewlines > 0) {
                // Text has explicit newlines - calculate height based on line count
                int numLines = explicitNewlines + 1;
                int lineSpacing = glyphProvider ? TextLayout::calculateLineSpacing(glyphProvider.get()) : 2;
                totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
                // Don't center horizontally for multi-line text
            } else if (textWidth <= this->frame.size.width) {
                // Single line: center horizontally
                horizontalOffset = (this->frame.size.width - textWidth) / 2;
                layoutWidth = this->frame.size.width - horizontalOffset;
            } else {
                // Multi-line due to wrapping: calculate number of lines needed for vertical centering
                int numLines = (textWidth + this->frame.size.width - 1) / this->frame.size.width;
                int lineSpacing = glyphProvider ? TextLayout::calculateLineSpacing(glyphProvider.get()) : 2;
                totalTextHeight = numLines * lineHeight + (numLines - 1) * lineSpacing;
            }

            int verticalOffset = (this->frame.size.height - totalTextHeight) / 2;
            Rect layoutRect = MakeRect(this->frame.origin.x + x + horizontalOffset, this->frame.origin.y + y + verticalOffset, layoutWidth, totalTextHeight);

            GlyphProvider* providerPtr = glyphProvider.get();
            if (this->focused) {
                display->fillRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                display->drawText(layoutRect, this->backgroundColor, 1, this->text.c_str(), providerPtr);
            } else {
                display->drawRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                display->drawText(layoutRect, this->foregroundColor, 1, this->text.c_str(), providerPtr);
            }
        }
    }
}

void Button::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

std::shared_ptr<Font> Button::getFont() const {
    return this->font;
}
