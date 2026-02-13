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

/**
 * @file CanvasView.hpp
 * @brief View with a 1bpp pixel buffer for programmatic drawing.
 *
 * CanvasView owns an in-memory 1-bit-per-pixel framebuffer that can be drawn
 * to programmatically using drawPixel(), drawRect(), fillRect(), drawCircle(),
 * fillCircle(), clear(), and drawText(). During the view draw cycle, the buffer
 * is blitted to the Display.
 *
 * CanvasView is used internally by many Focus views (LabelView, Button,
 * Checkbox, etc.) for off-screen rendering, and can also be used directly
 * for custom drawing (e.g. the EPUB renderer draws to a CanvasView).
 */

#pragma once

#include "View.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "GlyphProvider.hpp"
#include "UnicodeTraits.hpp"
#include <vector>
#include <memory>

/// A word's bounding box and byte range, recorded during text rendering.
/// Byte offsets are relative to the start of the UTF-8 string passed to drawText().
struct WordPosition {
    int16_t x, y;
    int16_t width, height;
    uint32_t startOffset;
    uint32_t endOffset;
};

/// A view that owns its own 1bpp pixel buffer for programmatic drawing.
/// Content is drawn to the internal buffer via drawPixel/drawRect/fillRect/drawText,
/// then blitted to the Display during the normal view draw cycle.
class CanvasView : public View {
public:
    CanvasView(Rect rect);

    void drawContent(int x, int y) override;

    // Drawing API — coordinates are local to the canvas (0,0 = top-left)
    void drawPixel(int x, int y, uint16_t color);
    void drawRect(int x, int y, int w, int h, uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawCircle(int cx, int cy, int r, uint16_t color);
    void fillCircle(int cx, int cy, int r, uint16_t color);
    void invertRect(int x, int y, int w, int h);
    void clear(uint16_t color);

    // Text rendering — renders text to the canvas buffer using the view's Font.
    // layoutRect is in canvas-local coordinates.
    int drawText(Rect layoutRect, uint16_t color, int textSize, const char *utf8String,
                 TextAlignment alignment = TextAlignmentLeft,
                 int initialEmphasisDepth = 0, int initialIndentLevel = 0);

    // Font property — if null, drawText uses Font::systemFont().
    void setFont(std::shared_ptr<Font> font);

    // Word map — when set, drawText() records each word's bounding box and
    // byte range into the provided vector (cleared before each drawText call).
    void setWordMapOutput(std::vector<WordPosition> *output);

    int getCanvasWidth() { return frame.size.width; }
    int getCanvasHeight() { return frame.size.height; }

    // Display mode — set to TwoBpp before drawing to enable 4-level grayscale.
    void setCanvasMode(DisplayMode mode);
    DisplayMode getCanvasMode() const { return canvasMode; }

    // Buffer access — for views that use a CanvasView internally and blit directly
    const uint8_t* getBufferData() const { return buffer.data(); }
    int getRowBytes() const { return rowBytes; }

private:
    int rowBytes;                 // bytes per row = (width + 7) / 8
    // 2bpp support — some displays use a two-plane buffer for 4-level grayscale.
    // In TwoBpp mode, plane 1 occupies the second half of buffer (offset planeSize).
    int planeSize;                // bytes per plane = rowBytes * height
    std::vector<uint8_t> buffer;  // OneBpp: planeSize bytes; TwoBpp: 2*planeSize (plane0 then plane1)
    const uint8_t* getPlane1Data() const { return buffer.data() + planeSize; }
    DisplayMode canvasMode = DisplayMode::OneBpp;

    // Private helper for fillRect — fills a single plane buffer
    void _fillPlane(uint8_t* plane, int x0, int y0, int x1, int y1, uint8_t fillByte);

    std::shared_ptr<Font> font;

    // Text rendering state (used during drawText)
    Point cursor = {};
    Rect textLayoutRect = {};
    int textSize = 1;
    uint16_t textColor = 0;
    int lineSpacing = 0;
    int paragraphSpacing = 0;
    int direction = 1;            // 1=LTR, -1=RTL
    int glyphRowCount = 0;
    Point lastGlyphPosition = {};
    bool hasLastGlyph = false;
    bool lastWasNewline = false;  // Tracks consecutive newlines for paragraph detection
    TextAlignment textAlignment = TextAlignmentLeft;

    // .text format emphasis state (SO/SI control codes)
    int emphasisDepth = 0;        // 0=normal, 1=italic, 2=bold, 3=bold+italic
    bool readingTitle = false;    // true after FS/GS/RS, until next newline
    int savedEmphasisDepth = 0;   // emphasis depth saved when entering title mode
    int initialIndentLevel = 0;   // Block quote indent level for mid-paragraph page starts

    // Word position tracking (set by setWordMapOutput, used during drawText)
    std::vector<WordPosition> *wordMapOutput = nullptr;
    uint32_t *codepointByteOffsets = nullptr;

    // Text rendering internals (mirror Display's pipeline)
    size_t writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider);
    int16_t measureCodepointsWidth(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider);
    size_t writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider);
    int drawGlyph(int16_t x, int16_t y, Rect glyphRect, unicode_info_t traits, uint8_t *glyph);
};
