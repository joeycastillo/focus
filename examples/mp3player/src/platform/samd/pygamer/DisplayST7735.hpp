#pragma once

#include "Display.hpp"
#include <Adafruit_ST7735.h>
#include <vector>

/// Focus Display backend for the PyGamer's 160x128 ST7735 TFT.
class DisplayST7735 : public focus::Display {
public:
    DisplayST7735();

    void fillRect(int x, int y, int w, int h, uint16_t color,
                  focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitOpaque(int x, int y, int w, int h,
                    const uint8_t* data, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitMasked(int x, int y, int w, int h, uint16_t color,
                    const uint8_t* mask, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;

    /// Push a framebuffer region to the panel. Zero-size rect = full screen.
    void flush(focus::Rect rect) override;

private:
    static constexpr int kWidth = 160;
    static constexpr int kHeight = 128;
    Adafruit_ST7735 tft;
    std::vector<uint16_t> pixels;  ///< RGB565, native byte order, row-major.
};
