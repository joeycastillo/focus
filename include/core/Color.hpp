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

#pragma once

#include <stdint.h>

namespace focus {

/**
 * @brief The canonical color type used throughout Focus.
 *
 * A 16-bit value representing a color. Interpretation is display-dependent,
 * and you're only meant to use one display type in a project. GrayscaleColor
 * is an intensity for monochromatic or grayscale displays; RGB565Color is an
 * RGB565 value on RGB/TFT displays. Aliased to uint16_t, so you can also use
 * raw integers to define colors if you prefer.
 * @ingroup core
 */
using Color = uint16_t;

/**
 * @brief Factory class for 16-bit grayscale color values.
 *
 * GrayscaleColor provides static methods that return uint16_t color values
 * using a 16-bit grayscale representation (0x0000 = black, 0xFFFF = white).
 *
 * **Bit depth conversion:**
 * - 8-bit: `color >> 8`  (0x00 to 0xFF)
 * - 4-bit: `color >> 12` (0x0 to 0xF)
 * - 3-bit: `color >> 13` (0x0 to 0x7)
 * - 2-bit: `color >> 14` (0x0 to 0x3)
 *
 * These values are for grayscale displays; they render incorrectly on RGB
 * displays. Use RGB565Color for RGB/TFT displays.
 * @ingroup core
 */
struct GrayscaleColor {
    static constexpr uint16_t Black()     { return 0x0000; }  // 0/255 intensity
    static constexpr uint16_t DarkGray()  { return 0x5555; }  // 85/255 intensity
    static constexpr uint16_t LightGray() { return 0xAAAA; }  // 170/255 intensity
    static constexpr uint16_t White()     { return 0xFFFF; }  // 255/255 intensity

};

/**
 * @brief Factory class for RGB565 color values.
 *
 * RGB565Color provides static methods that return uint16_t color values
 * in RGB565 format (5 bits red, 6 bits green, 5 bits blue). This is the
 * native format for most 16-bit TFT displays.
 *
 * These values are for RGB/TFT displays; they render incorrectly on grayscale
 * e-paper displays. Use GrayscaleColor for grayscale displays.
 * @ingroup core
 */
struct RGB565Color {
    // Primary colors
    static constexpr uint16_t Black()   { return 0x0000; }
    static constexpr uint16_t Red()     { return 0xF800; }
    static constexpr uint16_t Green()   { return 0x07E0; }
    static constexpr uint16_t Blue()    { return 0x001F; }
    static constexpr uint16_t White()   { return 0xFFFF; }

    // Secondary colors
    static constexpr uint16_t Yellow()  { return 0xFFE0; }  // Red + Green
    static constexpr uint16_t Cyan()    { return 0x07FF; }  // Green + Blue
    static constexpr uint16_t Magenta() { return 0xF81F; }  // Red + Blue

    // Grayscale values (equal intensity across R, G, B)
    static constexpr uint16_t DarkGray()  { return 0x52AA; }  // 85/255 intensity
    static constexpr uint16_t Gray()      { return 0x8410; }  // 128/255 intensity
    static constexpr uint16_t LightGray() { return 0xAD55; }  // 170/255 intensity

    /**
     * @brief Create an RGB565 color from 8-bit R, G, B components.
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @return RGB565-encoded color value
     */
    static constexpr uint16_t fromRGB(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    /**
     * @brief Create a grayscale RGB565 color from an 8-bit intensity value.
     * @param value Grayscale intensity (0-255, where 0=black, 255=white)
     * @return RGB565-encoded grayscale color
     */
    static constexpr uint16_t fromGrayscale(uint8_t value) {
        return fromRGB(value, value, value);
    }

};

}  // namespace focus
