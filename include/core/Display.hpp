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
 * Subclasses must implement fillRect() and blitMasked(). blitOpaque() has a
 * slow default implementation that decodes pixels via fillRect(); backends
 * should override it for performance.
 */

#pragma once

#include <stdint.h>
#include "Focus.hpp"

/// Display operating mode — describes the display's pixel capability.
enum class DisplayMode {
    Monochrome,   ///< 1 bit per pixel (black & white) — default
    Grayscale,    ///< 8-bit grayscale (one byte per pixel, 0x00–0xFF)
    RGB565        ///< 16-bit color (5 red, 6 green, 5 blue)
};

/**
 * @brief Abstract base class for display rendering backends.
 *
 * Subclasses must implement fillRect() and blitMasked(). blitOpaque() has
 * a default implementation that decodes pixels via fillRect(); backends
 * should override it for performance.
 *
 * Color values are uint16_t, produced by the GrayscaleColor or RGB565Color
 * factory classes (see Color.hpp). Display backends are responsible for
 * converting these 16-bit values to their native bit depth. Color.hpp
 * documents the standard bit-shift conversions (e.g. color >> 14 for 2-bit,
 * color >> 8 for 8-bit).
 * @ingroup core
 */
class Display {
public:
    /**
     * @brief Fill a rectangular region with a solid color.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color Fill color. Use GrayscaleColor or RGB565Color factory values.
     * @param clipRect Region to clip to. A zero-size rect means no clipping.
     */
    virtual void fillRect(int x, int y, int w, int h, uint16_t color,
                          Rect clipRect = {{0,0},{0,0}}) = 0;

    /**
     * @brief Blit a mode-dependent pixel buffer, overwriting all pixels.
     *
     * The data format depends on the current DisplayMode:
     * - Monochrome: 1bpp MSB-first. Set bits (1) = white, clear bits (0) = black.
     *   rowBytes = (width + 7) / 8.
     * - Grayscale: 8bpp, one byte per pixel (0x00 = black, 0xFF = white).
     *   rowBytes = width.
     * - RGB565: 16bpp, one uint16_t per pixel in platform-native byte order.
     *   rowBytes = width * 2. If the display controller expects a different byte
     *   order than the platform's native order, the backend is responsible for
     *   swapping — either in software in its blitOpaque() override, or via
     *   hardware byte-swap in the SPI/DMA configuration.
     *
     * The default implementation decodes pixels and calls fillRect() one pixel
     * at a time. Performance-sensitive backends should override this.
     *
     * @param x Left edge of the destination region.
     * @param y Top edge of the destination region.
     * @param w Width of the bitmap in pixels.
     * @param h Height of the bitmap in pixels.
     * @param data Pointer to the pixel data.
     * @param rowBytes Number of bytes per row in the source data.
     * @param clipRect Region to clip to. A zero-size rect means no clipping.
     */
    virtual void blitOpaque(int x, int y, int w, int h,
                            const uint8_t* data, int rowBytes,
                            Rect clipRect = {{0,0},{0,0}});

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
     * @param clipRect Region to clip to. A zero-size rect means no clipping.
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
    DisplayMode displayMode = DisplayMode::Monochrome;
    uint8_t rotation = 0;      ///< Rotation index: 0=0°, 1=90°, 2=180°, 3=270°.
    int nativeWidth = 0;       ///< Panel width in pixels (unrotated).
    int nativeHeight = 0;      ///< Panel height in pixels (unrotated).
};
