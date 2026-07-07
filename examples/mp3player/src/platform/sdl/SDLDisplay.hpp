#pragma once

#include "Display.hpp"
#include <vector>
#include <string>
#include <SDL.h>

/// SDL-backed RGB565 display. A native uint16_t buffer feeds an
/// SDL_PIXELFORMAT_RGB565 streaming texture; flush() presents it scaled
/// into the window.
class SDLDisplay : public focus::Display {
public:
    SDLDisplay(int width, int height, SDL_Renderer* renderer);
    ~SDLDisplay();

    void fillRect(int x, int y, int w, int h, uint16_t color,
                  focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitOpaque(int x, int y, int w, int h,
                    const uint8_t* data, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;
    void blitMasked(int x, int y, int w, int h, uint16_t color,
                    const uint8_t* mask, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override;

    /// Upload the pixel buffer and present it to the SDL window.
    void flush(focus::Rect dirtyRect) override;

    /// Save the current pixel buffer as a PNG file.
    bool savePNG(const std::string& filename);

private:
    int bufferWidth;
    int bufferHeight;
    std::vector<uint16_t> pixels;  ///< One uint16_t per pixel, RGB565 native order.
    SDL_Renderer* renderer;
    SDL_Texture* texture;
};
