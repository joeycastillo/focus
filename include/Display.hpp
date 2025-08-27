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
#include "utf8_decode.hpp"
#include "GlyphProvider.hpp"
#include "unicodetraits.hpp"

class Display {
public:
    virtual void drawPixel(int x, int y, int color) = 0;

    // virtual void drawLine(int x1, int y1, int x2, int y2, int color) = 0;

    virtual void drawRect(int x, int y, int w, int h, int color) = 0;
    virtual void fillRect(int x, int y, int w, int h, int color) = 0;
    // virtual void drawRoundRect(int x, int y, int w, int h, int r, int color);
    // virtual void fillRoundRect(int x, int y, int w, int h, int r, int color);
    // virtual void drawCircle(int x, int y, int r, int color);
    // virtual void fillCircle(int x, int y, int r, int color);
    // virtual void drawEllipse(int x, int y, int rx, int ry, int color);
    // virtual void fillEllipse(int x, int y, int rx, int ry, int color);

    // virtual void fillScreen(int color);

    int drawText(Rect layoutRect, int color, int text_size, const char * utf8String, GlyphProvider *glyphProvider = NULL);

    virtual int getBlackColor() = 0;
    virtual int getWhiteColor() = 0;

    virtual ~Display() {}

    void setDefaultGlyphProvider(std::shared_ptr<GlyphProvider> glyphProvider);
protected:
private:
    std::shared_ptr<GlyphProvider> defaultGlyphProvider = NULL;
    int8_t direction = 1;
    bool hasLastGlyph;
    Rect layoutRect;
    Point cursor;
    Point lastGlyphPosition;
    uint16_t textColor = 0;
    uint16_t textSize = 1;
    uint16_t lineSpacing = 0;
    uint16_t paragraphSpacing = 0;

    /*!
     @brief Writes a series of glyphs in the provided rect, wrapping as appropriate, advancing the line for newlines, and automatically changing the layout mode to RTL or LTR as appropriate.
     @param codepoints An array of codepoints that you wish to draw
     @param len The number of codepoints in the array
     @param glyphProvider The glyph provider offering glyph metrics and data for the string drawing operation.
     @returns the number of codepoints written
    */
    size_t writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider);

    /*!
     @brief Writes a glyph at the current internal cursor position
     @param codepoint The codepoint you wish to draw. Not UTF-8. Not UTF-16. The codepoint itself.
     @param glyphProvider The glyph provider offering glyph metrics and data for the string drawing operation.
     @returns the number 1 if a codepoint was written, 0 if one was not.
    */
    size_t writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider);

    /**
     @brief Method for determining where to word wrap lines
     @param buf A buffer of UNICODE_CODEPOINTS that you want to wrap.
     @param len number of codepoints in buf
     @param wrapped output variable, pointer to a boolean that we will set to true if we wrapped
     @param glyphProvider The glyph provider offering glyph metrics for the string layout operation.
     @return the position where a newline should be added in order to wrap to a given line length, or -1 if no newline is required.
     */
    int16_t word_wrap_position(UNICODE_CODEPOINT *buf, size_t len, bool *wrapped, GlyphProvider *glyphProvider);

    int drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph);
};
