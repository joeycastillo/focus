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
 * Bitmap operations use a 1-bit-per-pixel model with MSB-first byte packing.
 * In TwoBpp mode, two 1bpp planes encode 2 bits per pixel for 4-level grayscale.
 */

#pragma once

#include <stdint.h>

/// Display operating mode: 1 bit per pixel (black & white) or 2 bits per pixel
/// (4-level grayscale). In TwoBpp mode, pixel data is stored as two separate
/// 1bpp planes rather than a single packed 2bpp buffer.
enum class DisplayMode {
    OneBpp,   ///< 1 bit per pixel (black & white) — default
    TwoBpp    ///< 2 bits per pixel (4-level grayscale)
};

/**
 * @brief Abstract base class for display rendering backends.
 *
 * Subclasses must implement fillRect(), blitOpaque(), blitOpaque2bpp(),
 * and blitMasked(). The framework calls these methods during the view
 * draw cycle.
 *
 * Color values: 0 = black, 3 = white. In TwoBpp mode, 1 = dark gray and
 * 2 = light gray. In OneBpp mode, all nonzero values are treated as white.
 */
class Display {
public:
    /**
     * @brief Fill a rectangular region with a solid color.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color Fill color (0 = black, 3 = white; 1/2 for grays in TwoBpp).
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
     * @brief Blit two 1bpp planes as a 2bpp grayscale image.
     *
     * Each pixel's color is encoded across two planes:
     *   plane0 holds bit 1 (high bit) of the color value.
     *   plane1 holds bit 0 (low bit) of the color value.
     * Both planes use the same MSB-first packing as blitOpaque.
     *
     * @param x Left edge of the destination region.
     * @param y Top edge of the destination region.
     * @param w Width of the image in pixels.
     * @param h Height of the image in pixels.
     * @param plane0 Pointer to the 1bpp high-bit plane (MSB-first, row-major).
     * @param plane1 Pointer to the 1bpp low-bit plane (MSB-first, row-major).
     * @param rowBytes Number of bytes per row in each plane.
     */
    virtual void blitOpaque2bpp(int x, int y, int w, int h,
                                const uint8_t* plane0, const uint8_t* plane1,
                                int rowBytes) = 0;

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

    /// @brief Black color constant. Always 0.
    int getBlackColor() { return 0; }

    /// @brief White color constant. Always 3.
    /// In OneBpp mode, all nonzero values are treated as white by drawing code.
    int getWhiteColor() { return 3; }

    /// @brief Dark gray color constant (only meaningful in TwoBpp mode).
    int getDarkGrayColor() { return 1; }

    /// @brief Light gray color constant (only meaningful in TwoBpp mode).
    int getLightGrayColor() { return 2; }

    /**
     * @brief Set the display operating mode.
     *
     * Subclasses may override to perform hardware re-initialization
     * (e.g. loading a grayscale LUT for e-paper displays).
     */
    virtual void setDisplayMode(DisplayMode mode) { displayMode = mode; }

    /// @brief Get the current display mode.
    DisplayMode getDisplayMode() const { return displayMode; }

    /**
     * @brief Request that the next screen update be a full refresh.
     *
     * On e-paper displays this triggers a full black-white-black waveform to
     * clear ghosting artifacts. No-op by default for displays that don't need it.
     */
    virtual void forceFullRefresh() {}

    virtual ~Display() {}

protected:
    DisplayMode displayMode = DisplayMode::OneBpp;
};
