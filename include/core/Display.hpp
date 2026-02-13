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
#include "Focus.hpp"

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
    virtual void fillRect(int x, int y, int w, int h, uint16_t color,
                          Rect clipRect = {{0,0},{0,0}}) = 0;

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
                            const uint8_t* data, int rowBytes,
                            Rect clipRect = {{0,0},{0,0}}) = 0;

    /**
     * @brief Blit a 2bpp grayscale image stored as two contiguous 1bpp planes.
     *
     * Each pixel's color is encoded across two planes laid out consecutively
     * in memory: plane 0 (high bit) occupies the first rowBytes*h bytes,
     * followed immediately by plane 1 (low bit). Both planes use the same
     * MSB-first packing as blitOpaque.
     *
     * @param x Left edge of the destination region.
     * @param y Top edge of the destination region.
     * @param w Width of the image in pixels.
     * @param h Height of the image in pixels.
     * @param data Pointer to the two contiguous 1bpp planes (plane0 then plane1).
     * @param rowBytes Number of bytes per row in each plane.
     */
    virtual void blitOpaque2bpp(int x, int y, int w, int h,
                                const uint8_t* data, int rowBytes,
                                Rect clipRect = {{0,0},{0,0}}) = 0;

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
    virtual void blitMasked(int x, int y, int w, int h, uint16_t color,
                            const uint8_t* mask, int rowBytes,
                            Rect clipRect = {{0,0},{0,0}}) = 0;

    /// @brief Get the current display mode.
    DisplayMode getDisplayMode() const { return displayMode; }

    /**
     * @brief Set the display rotation.
     * @param degrees Rotation angle: 0, 90, 180, or 270.
     */
    virtual void setRotation(int degrees) {
        rotation = (degrees / 90) & 0x03;
    }

    /// @brief Get the rotation index (0=0°, 1=90°, 2=180°, 3=270°).
    uint8_t getRotation() const { return rotation; }

    /// @brief Get the display width accounting for rotation.
    int getWidth() const {
        return (rotation & 1) ? nativeHeight : nativeWidth;
    }

    /// @brief Get the display height accounting for rotation.
    int getHeight() const {
        return (rotation & 1) ? nativeWidth : nativeHeight;
    }

    /// @brief Get the native (unrotated) panel width.
    int getNativeWidth() const { return nativeWidth; }

    /// @brief Get the native (unrotated) panel height.
    int getNativeHeight() const { return nativeHeight; }

    virtual ~Display() {}

protected:
    DisplayMode displayMode = DisplayMode::OneBpp;
    uint8_t rotation = 0;      ///< Rotation index: 0=0°, 1=90°, 2=180°, 3=270°.
    int nativeWidth = 0;       ///< Panel width in pixels (unrotated).
    int nativeHeight = 0;      ///< Panel height in pixels (unrotated).
};
