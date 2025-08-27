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

#include "Display.hpp"
#include "GlyphProvider.hpp"
#include "utf8_parse.hpp"
#include <string.h>

int Display::drawText(int x, int y, int width, int height, int color, int text_size, const char * utf8String, GlyphProvider *glyphProvider) {
    if (glyphProvider == NULL) glyphProvider = this->defaultGlyphProvider.get();
    if (glyphProvider == NULL) return 0;


    size_t len = utf8_codepoint_length((char *)utf8String);

    this->textSize = text_size;
    this->textColor = color;
    this->lineSpacing = 2;
    this->paragraphSpacing = 8;
    this->layoutRect = MakeRect(x, y, width, height);

    UNICODE_CODEPOINT *codepoints = (UNICODE_CODEPOINT *)malloc(len * sizeof(UNICODE_CODEPOINT));

    utf8_parse((char *)utf8String, codepoints);
    size_t retVal = this->writeCodepoints(codepoints, len, glyphProvider);
    free(codepoints);

    return retVal;
}

size_t Display::writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider) {
    size_t retVal = 0;
    size_t pos = 0;
    this->cursor = this->layoutRect.origin;

    while (pos < len) {
        bool write_newline = false;
        bool wrapped = false;
        int32_t num_glyphs_to_draw = this->word_wrap_position(codepoints + pos, len - pos, &wrapped, glyphProvider);
        if (num_glyphs_to_draw < 0){
            num_glyphs_to_draw = (int32_t)(len - pos);
        }
        else {
            write_newline = true;
        }
        for(size_t i = pos; i < pos + num_glyphs_to_draw; i++) {
            retVal += this->writeCodepoint(codepoints[i], glyphProvider);
        }
        pos += num_glyphs_to_draw;
        if (write_newline && wrapped) {
            this->cursor.y += 16 * this->textSize; /// TODO: + this->lineSpacing;
            if (this->direction == 1) {
                this->cursor.x = this->layoutRect.origin.x;
            } else {
                this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width;
            }
        }

        if (this->cursor.y >= (this->layoutRect.origin.y + this->layoutRect.size.height)) break;
    }

    return retVal;
}

size_t Display::writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider) {
    // before we start, we don't need to fetch anything for control characters.
    if (codepoint == '\n' || codepoint == '\r') {
        this->cursor.y += 16 * this->textSize + this->paragraphSpacing;
        if (this->direction == 1) {
            this->cursor.x = this->layoutRect.origin.x;
        } else {
            this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width;
        }
        return 1;
    }
    // skip all control characters
    if (codepoint < 0x20) return 1;

    unicode_info_t traits = getTraitsForCodepoint(codepoint);
    Rect metrics = glyphProvider->metricsForCodepoint(codepoint);

    if (this->direction == 1 && traits.is.rtl) {
        direction = -1;
        uint8_t width = metrics.size.width;
        this->hasLastGlyph = false;
        this->cursor.x = this->layoutRect.origin.x + this->layoutRect.size.width - width;
    }
    else if (this->direction == -1 && traits.is.ltr) {
        direction = 1;
        this->hasLastGlyph = false;
        this->cursor.x = this->layoutRect.origin.x;
    }

    uint8_t *glyph = glyphProvider->glyphForCodepoint(codepoint);
    // word wrap should go here
    if (traits.is.nsm && this->hasLastGlyph) {
        // Draw over the last glyph, and do not add to advance
        drawGlyph(this->lastGlyphPosition.x, this->lastGlyphPosition.y, metrics, traits, glyph);
    } else {
        // stash current cursor position
        this->hasLastGlyph = true;
        this->lastGlyphPosition = this->cursor;
        // draw glyph
        int advance = drawGlyph(this->cursor.x, this->cursor.y, metrics, traits, glyph);
        // advance cursor
        this->cursor.x += advance * this->direction;
    }

    return 1;
}

int16_t Display::word_wrap_position(UNICODE_CODEPOINT *buf, size_t len, bool *wrapped, GlyphProvider *glyphProvider) {
    size_t wrap_candidate = 0;
    size_t byte_position = 0;
    size_t position_in_string = 0;
    int16_t cursor_location = 0;
    *wrapped = true; // assume we wrapped unless set otherwise below
    
    while(cursor_location < this->layoutRect.size.width) {
        if (buf[position_in_string] == '\n') {
            *wrapped = false;
            return position_in_string + 1; // "wrap" at the newline
        }
        // skip control characters
        if (buf[position_in_string] < 0x20) {
            byte_position++;
            position_in_string++;
            continue;
        }

        unicode_info_t traits = getTraitsForCodepoint(buf[position_in_string]);
        Rect metrics = glyphProvider->metricsForCodepoint(buf[position_in_string]);

        if (traits.is.linebreak) {
            wrap_candidate = position_in_string;
        }
        if (!(traits.is.nsm || traits.is.controlchar)) {
            cursor_location += metrics.size.width * this->textSize;
        }
#ifndef UNICODE_BMP_ONLY
        if (buf[position_in_string] > 0x00ffff) byte_position++;
#endif
        if (buf[position_in_string] > 0x0007ff) byte_position++;
        if (buf[position_in_string] > 0x00007f) byte_position++;
        byte_position++;
        position_in_string++;
        if (position_in_string >= len) {
            *wrapped = false;
            // FIXME: byte position and wrap position?
            return -1; // we didn't have to word wrap
        }
    }
    
    if (wrap_candidate) {
        return wrap_candidate + 1; // if we found a wrap point, return it (plus the space after).
    } else {
        return len; // otherwise, they'll just need to break at the end of the line even though it's in the middle of a word.
    }
}

int Display::drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph) {
    uint8_t width = glyphRect.size.width;
    uint8_t characterWidth = (width + 7) / 8;
    bool mirrored = (this->direction == -1) && traits.is.mirrored;

    if (mirrored) {
        switch (characterWidth) {
            case 1:
                for(int8_t i=0; i<characterWidth*16; i++ ) {
                    uint8_t line = glyph[i];
                    for(int8_t j=7; j>= 0; j--, line >>= 1) {
                        if(line & 1) {
                            if(this->textSize == 1) drawPixel(x+8-j, y+i, this->textColor);
                            else this->fillRect(x+8*this->textSize-j*this->textSize, y+i*this->textSize, this->textSize, this->textSize, this->textColor);
                        }
                    }
                }
                break;
            case 2:
                for(int8_t i=0; i<characterWidth*16; i++ ) {
                    uint8_t line = glyph[i];
                    for(int8_t j=7; j>= 0; j--, line >>= 1) {
                        if(line & 1) {
                            if(this->textSize == 1) drawPixel(x+8-(j+(i%2?8:0)), y+i/2, this->textColor);
                            else this->fillRect(x+8*this->textSize-(j+(i%2?8:0))*this->textSize, y+(i/2)*this->textSize, this->textSize, this->textSize, this->textColor);
                        }
                    }
                }
                break;
        }
    } else {
        switch (characterWidth) {
            case 1:
                for(int8_t i=0; i<characterWidth*16; i++ ) {
                    uint8_t line = glyph[i];
                    for(int8_t j=7; j>= 0; j--, line >>= 1) {
                        if(line & 1) {
                            if(this->textSize == 1) drawPixel(x+j, y+i, this->textColor);
                            else this->fillRect(x+j*this->textSize, y+i*this->textSize, this->textSize, this->textSize, this->textColor);
                        }
                    }
                }
                break;
            case 2:
                for(int8_t i=0; i<characterWidth*16; i++ ) {
                    uint8_t line = glyph[i];
                    for(int8_t j=7; j>= 0; j--, line >>= 1) {
                        if(line & 1) {
                            if(this->textSize == 1) drawPixel(x+j+(i%2?8:0), y+i/2, this->textColor);
                            else this->fillRect(x+(j+(i%2?8:0))*this->textSize, y+(i/2)*this->textSize, this->textSize, this->textSize, this->textColor);
                        }
                    }
                }
                break;
        }
    }

    return width * this->textSize;
}

void Display::setDefaultGlyphProvider(std::shared_ptr<GlyphProvider> glyphProvider) {
    this->defaultGlyphProvider = glyphProvider;
}
