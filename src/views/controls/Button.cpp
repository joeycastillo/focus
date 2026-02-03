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
#include <vector>

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

            // Check if text has explicit newlines
            bool hasNewlines = this->text.find('\n') != std::string::npos;

            // Draw button background/border
            GlyphProvider* providerPtr = glyphProvider.get();
            uint16_t textColor;
            if (this->focused) {
                display->fillRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                textColor = this->backgroundColor;
            } else {
                display->drawRect(x + this->frame.origin.x, y + this->frame.origin.y, this->frame.size.width, this->frame.size.height, this->foregroundColor);
                textColor = this->foregroundColor;
            }

            if (hasNewlines) {
                // Text has explicit newlines - draw each line centered individually
                std::vector<std::string> lines;
                std::vector<int> lineWidths;
                size_t lineStart = 0;
                for (size_t i = 0; i <= this->text.length(); i++) {
                    if (i == this->text.length() || this->text[i] == '\n') {
                        std::string line = (i > lineStart) ? this->text.substr(lineStart, i - lineStart) : "";
                        lines.push_back(line);
                        int lineWidth = glyphProvider ? TextLayout::measureTextWidth(line.c_str(), 1, glyphProvider.get()) : 0;
                        lineWidths.push_back(lineWidth);
                        lineStart = i + 1;
                    }
                }

                int lineSpacing = glyphProvider ? TextLayout::calculateLineSpacing(glyphProvider.get()) : 2;
                int totalTextHeight = lines.size() * lineHeight + (lines.size() - 1) * lineSpacing;
                int verticalOffset = (this->frame.size.height - totalTextHeight) / 2;

                int lineY = this->frame.origin.y + y + verticalOffset;
                for (size_t i = 0; i < lines.size(); i++) {
                    int horizontalOffset = (this->frame.size.width - lineWidths[i]) / 2;
                    if (horizontalOffset < 0) horizontalOffset = 0;
                    int availableWidth = this->frame.size.width - horizontalOffset;
                    Rect lineRect = MakeRect(this->frame.origin.x + x + horizontalOffset, lineY, availableWidth, lineHeight);
                    display->drawText(lineRect, textColor, 1, lines[i].c_str(), providerPtr);
                    lineY += lineHeight + lineSpacing;
                }
            } else {
                // No newlines - use standard text rendering with word wrap support
                int horizontalOffset = 0;
                int totalTextHeight = lineHeight;
                int layoutWidth = this->frame.size.width;

                if (textWidth <= this->frame.size.width) {
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
                display->drawText(layoutRect, textColor, 1, this->text.c_str(), providerPtr);
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
