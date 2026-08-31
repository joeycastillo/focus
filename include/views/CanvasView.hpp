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
 * @brief View with an owned pixel buffer for programmatic drawing.
 *
 * CanvasView owns an in-memory framebuffer that can be drawn to programmatically
 * using drawPixel(), drawRect(), fillRect(), drawCircle(), fillCircle(), clear(),
 * and drawText(). During the view draw cycle the buffer is blitted to the Display.
 *
 * The buffer format depends on the canvas mode (set via setCanvasMode()):
 * - Monochrome (default): 1bpp MSB-first, rowBytes = (width+7)/8
 * - Grayscale: 8bpp, one byte per pixel (0x00=black, 0xFF=white), rowBytes = width
 * - RGB565: 16bpp, one uint16_t per pixel in platform-native byte order, rowBytes = width*2
 *
 * CanvasView is used internally by many Focus views (LabelView, Button,
 * Checkbox, etc.) for off-screen rendering, and can also be used directly
 * for custom drawing (e.g. a charting view or custom visualization).
 */

#pragma once

#include "View.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "GlyphProvider.hpp"
#include "UnicodeTraits.hpp"
#include <vector>
#include <memory>

namespace focus {

/// A word's bounding box and byte range, recorded during text rendering.
/// Byte offsets are relative to the start of the UTF-8 string passed to drawText().
struct WordPosition {
    int16_t x, y;
    int16_t width, height;
    uint32_t startOffset;
    uint32_t endOffset;
};

/// A view that owns its own pixel buffer for programmatic drawing.
/// Content is drawn to the internal buffer via drawPixel/drawRect/fillRect/drawText,
/// then blitted to the Display during the normal view draw cycle.
///
/// @par Memory
/// The buffer is retained for the lifetime of the view (or until setFrame()
/// resizes it). Hiding a CanvasView with setHidden(true) prevents drawing
/// but does not release the buffer — use this when the view will reappear
/// (e.g., toggling an indicator). To release the buffer, remove the view
/// from the hierarchy with removeSubview().
class CanvasView : public View {
public:
    /// Construct a canvas with the given frame. If either dimension is zero
    /// or negative, buffer allocation is deferred until setFrame() assigns
    /// positive dimensions (e.g., during a StackView layout pass). Drawing
    /// operations on a zero-size canvas are safe no-ops.
    CanvasView(Rect rect);
    void setFrame(Rect rect) override;

    void drawContent(int x, int y, Rect clipRect = {{0,0},{0,0}}) override;

    // Drawing API — coordinates are local to the canvas (0,0 = top-left)
    void drawPixel(int x, int y, uint16_t color);
    void drawRect(int x, int y, int w, int h, uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawCircle(int cx, int cy, int r, uint16_t color);
    void fillCircle(int cx, int cy, int r, uint16_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
    void invertRect(int x, int y, int w, int h);
    void clear(uint16_t color);

    /// @brief Stamp 1bpp mask data into the canvas. Where mask bits are set,
    /// the specified color is drawn. Mask format: MSB-first, packed rows.
    void drawMask(int x, int y, int w, int h,
                  const uint8_t* mask, int maskRowBytes, uint16_t color);

    /// Apply a 1px checkerboard mask, setting every other pixel to the given color.
    /// Used to render a "disabled" appearance. The pattern alternates per pixel in
    /// both axes: even rows mask with 0xAA, odd rows with 0x55 (MSB-first).
    /// In Grayscale mode, both planes are updated.
    void applyCheckerboardMask(uint16_t color);

    /// Draw text with automatic word-wrapping and layout.
    /// Handles the full pipeline internally: UTF-8 decoding, word-wrapping,
    /// bidi reordering, paragraph spacing, and glyph rendering. Suitable for
    /// UI text (labels, buttons) where layout and rendering happen together.
    /// layoutRect is in canvas-local coordinates.
    /// @return The Y position after the last line (for stacking content below).
    /// @note Allocates temporary heap memory proportional to the input
    ///       string length (approximately 8-9 bytes per codepoint).
    ///       For long text, split the input across multiple drawText
    ///       calls rather than passing the entire string at once.
    int drawText(Rect layoutRect, uint16_t color, int textSize, const char *utf8String,
                 TextAlignment alignment = TextAlignment::Left);

    // Font property — if null, drawText uses Font::systemFont().
    void setFont(std::shared_ptr<Font> font);

    // Word map — when set, drawText() records each word's bounding box and
    // byte range into the provided vector (cleared before each drawText call).
    void setWordMapOutput(std::vector<WordPosition> *output);

    // Truncation — when Tail, drawText() ends the last line that fits
    // vertically with an ellipsis instead of clipping the overflow.
    void setTruncationMode(TruncationMode mode);
    TruncationMode getTruncationMode() const { return truncationMode; }

    /// Get the logical canvas width (accounts for canvas rotation).
    /// For 0°/180° this is the frame width; for 90°/270° it is the frame height.
    int getCanvasWidth() const { return (canvasRotation & 1) ? frame.size.height : frame.size.width; }
    /// Get the logical canvas height (accounts for canvas rotation).
    /// For 0°/180° this is the frame height; for 90°/270° it is the frame width.
    int getCanvasHeight() const { return (canvasRotation & 1) ? frame.size.width : frame.size.height; }

    /// Set the canvas content rotation (0, 90, 180, or 270 degrees).
    /// This rotates the drawing coordinate space within the canvas buffer.
    /// The view's frame, hit testing, and child layout are unaffected —
    /// only drawing operations (drawPixel, drawText, etc.) are remapped.
    void setCanvasRotation(int degrees);
    /// Get the canvas content rotation in degrees (0, 90, 180, or 270).
    int getCanvasRotation() const { return canvasRotation * 90; }

    // Display mode — set to Grayscale before drawing to enable 4-level grayscale.
    void setCanvasMode(DisplayMode mode);
    DisplayMode getCanvasMode() const { return canvasMode; }

    // Buffer access — for views that use a CanvasView internally and blit directly
    const uint8_t* getBufferData() const { return buffer.data(); }
    uint8_t* getMutableBufferData() { return buffer.data(); }
    int getRowBytes() const { return rowBytes; }

protected:
    /// @name Subclass text rendering API
    /// These members are protected so that subclasses can implement custom text
    /// rendering strategies (e.g. rendering pre-laid-out paginated content).
    /// The contract: set the state variables below, then call renderBidiLine()
    /// once per visual line. The base class handles bidi reordering, alignment,
    /// emphasis rendering, and glyph drawing internally.
    /// @{

    std::shared_ptr<Font> font;

    // Text rendering state — set these before calling renderBidiLine.
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
    bool lastGlyphWasUnderscore = false;     // Tracks _ for overprint underline detection
    bool pendingOverprintUnderline = false;  // Set when BS follows _, triggers underscore redraw
    bool lastWasNewline = false;  // Tracks consecutive newlines for paragraph detection
    TextAlignment textAlignment = TextAlignment::Left;
    TruncationMode truncationMode = TruncationMode::None;

    // Emphasis depth tracked from SO/SI control codes (0-3). static_cast to
    // FontStyle for glyph queries (0=regular, 1=italic, 2=bold, 3=bold+italic).
    uint8_t emphasisDepth = 0;

    // Word position tracking (set by setWordMapOutput, used during text rendering)
    std::vector<WordPosition> *wordMapOutput = nullptr;
    uint32_t *codepointByteOffsets = nullptr;

    /// Measure the pixel width of a codepoint array (for text alignment).
    int16_t measureCodepointsWidth(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider);

    /// Render one visual line with bidi reordering, alignment, and glyph drawing.
    /// Set cursor.y and emphasisDepth before calling. The method handles LTR/RTL
    /// reordering, text alignment, word position tracking, and glyph rendering.
    void renderBidiLine(UNICODE_CODEPOINT *codepoints, size_t lineStart, size_t lineLen,
                        int paragraphDir, int16_t effectiveWidth, int16_t indentedOriginX,
                        GlyphProvider *glyphProvider);

    /// @}

    /// Emit a single glyph at the current cursor position and advance the cursor.
    size_t writeCodepoint(UNICODE_CODEPOINT codepoint, GlyphProvider *glyphProvider);

    /// Render the remaining text as the longest prefix that fits with an
    /// ellipsis appended, as one line through renderBidiLine. Called from
    /// writeCodepoints in TruncationMode::Tail for the last fitting line.
    void renderTruncatedLine(UNICODE_CODEPOINT *codepoints, size_t len, GlyphProvider *glyphProvider);

private:
    int rowBytes;                 // Monochrome: (width+7)/8; Grayscale: width; RGB565: width*2
    int planeSize;                // Monochrome buffer size = ((width+7)/8) * height
    std::vector<uint8_t> buffer;  // Monochrome: 1bpp; Grayscale: 8bpp; RGB565: 16bpp platform-native uint16_t
    std::vector<uint8_t> bidiResolveBuffer; // Reusable buffer for bidi character class resolution
    DisplayMode canvasMode = DisplayMode::Monochrome;
    int canvasRotation = 0;       // rotation index: 0=0°, 1=90°, 2=180°, 3=270°

    /// Map logical (pre-rotation) coordinates to physical buffer coordinates.
    void mapToBuffer(int x, int y, int &bx, int &by) const;

    // Private helper for fillRect — fills a single plane buffer
    void _fillPlane(uint8_t* plane, int x0, int y0, int x1, int y1, uint8_t fillByte);

    // Text rendering internals (mirror Display's pipeline)
    size_t writeCodepoints(UNICODE_CODEPOINT codepoints[], size_t len, GlyphProvider *glyphProvider);
    int drawGlyph(int16_t x, int16_t y, GlyphMetrics glyphRect, unicode_info_t traits, const uint8_t *glyph);
};

}  // namespace focus
