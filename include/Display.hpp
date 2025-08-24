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

    int drawText(int x, int y, int width, int height, int color, int text_size, const char * utf8String, GlyphProvider *glyphProvider = NULL);

    virtual int getBlackColor() = 0;
    virtual int getWhiteColor() = 0;

    virtual ~Display() {}

    void setDefaultGlyphProvider(std::shared_ptr<GlyphProvider> glyphProvider);
protected:
private:
    std::shared_ptr<GlyphProvider> defaultGlyphProvider = NULL;
    int8_t direction = 1;
    bool hasLastGlyph;
    Point cursor;
    Point lastGlyphPosition;
    uint16_t textColor = 0;
    uint16_t textSize = 1;
    uint16_t lineSpacing = 0;
    uint16_t paragraphSpacing = 0;
    /**
     @brief This method takes a NULL-terminated UTF-8 string and returns its length in codepoints. Should be O(n) time.
     @return the number of codepoints required to represent this string, or 0 if the string was invalid.
     @param string a pointer to a a UTF-8 string
     */
    size_t utf8_codepoint_length(char * string);

    /**
     @brief This method takes a NULL-terminated UTF-8 string and parses it into codepoints, which it places in the buffer pointed to by buf. O(N) time.
     @return the number of codepoints required to represent this string, or 0 if the string was invalid.
     @param string a pointer to a a UTF-8 string
     @param buf output parameter, a pointer to a buffer that will receive the parsed codepoints.
     */
    size_t utf8_parse(char * string, UNICODE_CODEPOINT *buf);

    /*!
     @brief Writes a series of glyphs in the provided rect. It will not currently wrap, but will advance the line for newlines, and automatically change the layout mode to RTL or LTR as appropriate.
     @param codepoints An array of codepoints that you wish to draw
     @param len The number of codepoints in the array
     @returns the number of codepoints written
     @note This method handles newlines and direction changes, and updates the current cursor position. It might move 8 or 16 pixels to the right, OR it might move to the left side of the next line if the text wrapped. But it could also move to the right side of the next line if the layout direction changed to RTL mode.
    */
    size_t writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, Rect rect, GlyphProvider *glyphProvider);

    /*!
     @brief Writes a glyph at the given point
     @param codepoint The codepoint you wish to draw. Not UTF-8. Not UTF-16. The codepoint itself.
     @returns the number 1 if a codepoint was written, 0 if one was not.
     @note This method handles newlines and direction changes, and updates the current cursor position. It might move 8 or 16 pixels to the right, OR it might move to the left side of the next line if the text wrapped. But it could also move to the right side of the next line if the layout direction changed to RTL mode.
    */
    size_t writeCodepoint(UNICODE_CODEPOINT codepoint, Rect layoutRect, GlyphProvider *glyphProvider);

    /**
     @brief Method for determining where to word wrap lines
     @param buf A buffer of BABEL_CODEPOINTS that you want to wrap.
     @param len number of codepoints in buf
     @param line_width the width in pixels that you want to wrap to
     @param text_size scaling factor for the text (1 for 1x, 2 for 2x, etc.)
     @return the position where a newline should be added in order to wrap to a given line length, or -1 if no newline is required.
     */
    int16_t word_wrap_position(UNICODE_CODEPOINT *buf, size_t len, bool *wrapped, size_t *wrap_candidate_bytes, int16_t line_width, GlyphProvider *glyphProvider);

    int drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph);
};
