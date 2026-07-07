#pragma once

#include "Display.hpp"
#include <Adafruit_ILI9341.h>

/// Focus Display backend for the PyPortal's 320x240 ILI9341 on the 8-bit
/// parallel bus. Draws directly to panel GRAM — no framebuffer, so the
/// base class's no-op flush() is correct as-is.
class DisplayILI9341 : public focus::Display {
public:
    DisplayILI9341();

    void fillRect(int x, int y, int w, int h, uint16_t color,
                  focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitOpaque(int x, int y, int w, int h,
                    const uint8_t* data, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitMasked(int x, int y, int w, int h, uint16_t color,
                    const uint8_t* mask, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;

private:
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 240;
    Adafruit_ILI9341 tft;
};
