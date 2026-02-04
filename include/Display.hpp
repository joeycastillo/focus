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

/**
 * @file Display.hpp
 * @brief Abstract display interface for the Focus UI framework.
 *
 * Display defines the rendering contract that platform-specific display drivers
 * must implement. Focus views render through this interface, making the framework
 * independent of any particular display technology (e-paper, LCD, SDL, etc.).
 *
 * All bitmap operations use a 1-bit-per-pixel model with MSB-first byte packing.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Abstract base class for display rendering backends.
 *
 * Subclasses must implement fillRect(), blitOpaque(), blitMasked(),
 * getBlackColor(), and getWhiteColor(). The framework calls these
 * methods during the view draw cycle.
 */
class Display {
public:
    /**
     * @brief Fill a rectangular region with a solid color.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color Fill color (use getBlackColor() or getWhiteColor()).
     */
    virtual void fillRect(int x, int y, int w, int h, int color) = 0;

    /**
     * @brief Blit a 1bpp MSB-first bitmap, overwriting all pixels in the region.
     *
     * Set bits (1) write white; clear bits (0) write black. Every pixel in the
     * destination rectangle is written.
     *
     * @param x Left edge of the destination region.
     * @param y Top edge of the destination region.
     * @param w Width of the bitmap in pixels.
     * @param h Height of the bitmap in pixels.
     * @param data Pointer to the 1bpp bitmap data (MSB-first, row-major).
     * @param rowBytes Number of bytes per row in the source data.
     */
    virtual void blitOpaque(int x, int y, int w, int h,
                            const uint8_t* data, int rowBytes) = 0;

    /**
     * @brief Write a solid color only where mask bits are set.
     *
     * Pixels where the mask bit is 0 are left unchanged. This is used for
     * rendering glyph bitmaps and icons with transparency.
     *
     * @param x Left edge of the destination region.
     * @param y Top edge of the destination region.
     * @param w Width of the mask in pixels.
     * @param h Height of the mask in pixels.
     * @param color Color to write where mask bits are set.
     * @param mask Pointer to the 1bpp mask data (MSB-first, row-major).
     * @param rowBytes Number of bytes per row in the mask data.
     */
    virtual void blitMasked(int x, int y, int w, int h, int color,
                            const uint8_t* mask, int rowBytes) = 0;

    /**
     * @brief Get the display's black color value.
     * @return Platform-specific color value representing black.
     */
    virtual int getBlackColor() = 0;

    /**
     * @brief Get the display's white color value.
     * @return Platform-specific color value representing white.
     */
    virtual int getWhiteColor() = 0;

    /**
     * @brief Request that the next screen update be a full refresh.
     *
     * On e-paper displays this triggers a full black-white-black waveform to
     * clear ghosting artifacts. No-op by default for displays that don't need it.
     */
    virtual void forceFullRefresh() {}

    virtual ~Display() {}
};
