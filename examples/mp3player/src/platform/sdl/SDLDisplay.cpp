#include "platform/sdl/SDLDisplay.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "platform/sdl/stb_image_write.h"
#include <algorithm>
#include <cstring>

using focus::Rect;

SDLDisplay::SDLDisplay(int width, int height, SDL_Renderer* renderer)
    : bufferWidth(width), bufferHeight(height),
      pixels(width * height, 0), renderer(renderer) {
    this->displayMode = focus::DisplayMode::RGB565;
    this->nativeWidth = width;
    this->nativeHeight = height;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                SDL_TEXTUREACCESS_STREAMING, width, height);
}

SDLDisplay::~SDLDisplay() {
    if (texture) SDL_DestroyTexture(texture);
}

void SDLDisplay::fillRect(int x, int y, int w, int h, uint16_t color, Rect clipRect) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(bufferWidth, x + w);
    int y1 = std::min(bufferHeight, y + h);
    if (x0 >= x1 || y0 >= y1) return;

    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        x0 = std::max(x0, clipRect.origin.x);
        y0 = std::max(y0, clipRect.origin.y);
        x1 = std::min(x1, clipRect.origin.x + clipRect.size.width);
        y1 = std::min(y1, clipRect.origin.y + clipRect.size.height);
        if (x0 >= x1 || y0 >= y1) return;
    }

    int spanW = x1 - x0;
    if ((color >> 8) == (color & 0xFF)) {
        // Both bytes identical (e.g. 0x0000, 0xFFFF) — memset whole spans.
        uint8_t byte = color & 0xFF;
        for (int row = y0; row < y1; row++) {
            memset(reinterpret_cast<uint8_t*>(pixels.data() + row * bufferWidth + x0),
                   byte, spanW * 2);
        }
    } else {
        // Fill first row with a loop, then memcpy into remaining rows.
        uint16_t* firstRow = pixels.data() + y0 * bufferWidth + x0;
        for (int col = 0; col < spanW; col++) firstRow[col] = color;
        for (int row = y0 + 1; row < y1; row++) {
            memcpy(pixels.data() + row * bufferWidth + x0, firstRow, spanW * sizeof(uint16_t));
        }
    }
}

void SDLDisplay::blitOpaque(int x, int y, int w, int h,
                            const uint8_t* data, int rowBytes, Rect clipRect) {
    int sx0 = 0, sy0 = 0, sx1 = w, sy1 = h;
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        sx0 = std::max(0, clipRect.origin.x - x);
        sy0 = std::max(0, clipRect.origin.y - y);
        sx1 = std::min(w, clipRect.origin.x + clipRect.size.width - x);
        sy1 = std::min(h, clipRect.origin.y + clipRect.size.height - y);
        if (sx0 >= sx1 || sy0 >= sy1) return;
    }

    for (int sy = sy0; sy < sy1; sy++) {
        int dy = y + sy;
        if (dy < 0 || dy >= bufferHeight) continue;

        const uint16_t* srcPixel = reinterpret_cast<const uint16_t*>(data + sy * rowBytes) + sx0;
        int dx = x + sx0;
        int count = sx1 - sx0;

        if (dx < 0) { srcPixel += (-dx); count += dx; dx = 0; }
        if (dx + count > bufferWidth) count = bufferWidth - dx;
        if (count <= 0) continue;

        memcpy(pixels.data() + dy * bufferWidth + dx, srcPixel, count * sizeof(uint16_t));
    }
}

void SDLDisplay::blitMasked(int x, int y, int w, int h, uint16_t color,
                            const uint8_t* mask, int rowBytes, Rect clipRect) {
    int sx0 = 0, sy0 = 0, sx1 = w, sy1 = h;
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        sx0 = std::max(0, clipRect.origin.x - x);
        sy0 = std::max(0, clipRect.origin.y - y);
        sx1 = std::min(w, clipRect.origin.x + clipRect.size.width - x);
        sy1 = std::min(h, clipRect.origin.y + clipRect.size.height - y);
        if (sx0 >= sx1 || sy0 >= sy1) return;
    }

    for (int sy = sy0; sy < sy1; sy++) {
        int dy = y + sy;
        if (dy < 0 || dy >= bufferHeight) continue;
        for (int sx = sx0; sx < sx1; sx++) {
            int dx = x + sx;
            if (dx < 0 || dx >= bufferWidth) continue;
            bool maskBit = mask[sy * rowBytes + (sx >> 3)] & (0x80 >> (sx & 7));
            if (maskBit) {
                pixels[dy * bufferWidth + dx] = color;
            }
        }
    }
}

void SDLDisplay::flush(Rect) {
    SDL_UpdateTexture(texture, NULL, pixels.data(), bufferWidth * sizeof(uint16_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool SDLDisplay::savePNG(const std::string& filename) {
    std::vector<uint8_t> rgb(bufferWidth * bufferHeight * 3);
    for (int i = 0; i < bufferWidth * bufferHeight; i++) {
        uint16_t px = pixels[i];
        uint8_t r5 = (px >> 11) & 0x1F;
        uint8_t g6 = (px >> 5)  & 0x3F;
        uint8_t b5 =  px        & 0x1F;
        // Scale to 8-bit by replicating top bits into the vacated low bits.
        rgb[i * 3 + 0] = (r5 << 3) | (r5 >> 2);
        rgb[i * 3 + 1] = (g6 << 2) | (g6 >> 4);
        rgb[i * 3 + 2] = (b5 << 3) | (b5 >> 2);
    }
    return stbi_write_png(filename.c_str(), bufferWidth, bufferHeight, 3,
                          rgb.data(), bufferWidth * 3) != 0;
}
