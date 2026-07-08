// tests/RecordingDisplay.hpp
/*
 * Recording Display stub for unit tests.
 *
 * Records every pixel operation with its clip rect, and composites into a
 * byte-per-pixel framebuffer (fills write color & 0xFF; masked blits write 1
 * where mask bits are set) so tests can assert both call parameters and
 * final pixel placement.
 */

#pragma once

#include "Display.hpp"
#include <vector>
#include <algorithm>

class RecordingDisplay : public focus::Display {
public:
    struct Op {
        enum class Type { FillRect, BlitMasked, BlitOpaque } type;
        int x, y, w, h;
        uint16_t color;
        focus::Rect clip;
    };

    RecordingDisplay(int width, int height) {
        this->nativeWidth = width;
        this->nativeHeight = height;
        this->framebuffer.assign((size_t)width * height, 0);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color,
                  focus::Rect clipRect = {{0,0},{0,0}}) override {
        this->ops.push_back({Op::Type::FillRect, x, y, w, h, color, clipRect});
        for (int py = y; py < y + h; py++)
            for (int px = x; px < x + w; px++)
                if (inClip(px, py, clipRect)) plot(px, py, (uint8_t)(color & 0xFF));
    }

    void blitMasked(int x, int y, int w, int h, uint16_t color,
                    const uint8_t* mask, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override {
        this->ops.push_back({Op::Type::BlitMasked, x, y, w, h, color, clipRect});
        for (int sy = 0; sy < h; sy++)
            for (int sx = 0; sx < w; sx++) {
                if (!(mask[sy * rowBytes + sx / 8] & (0x80 >> (sx % 8)))) continue;
                if (inClip(x + sx, y + sy, clipRect)) plot(x + sx, y + sy, 1);
            }
    }

    void blitOpaque(int x, int y, int w, int h, const uint8_t* data, int rowBytes,
                    focus::Rect clipRect = {{0,0},{0,0}}) override {
        this->ops.push_back({Op::Type::BlitOpaque, x, y, w, h, 0, clipRect});
        // 1bpp decode: set bits plot 1 (enough for the monochrome canvas tests).
        for (int sy = 0; sy < h; sy++)
            for (int sx = 0; sx < w; sx++) {
                uint8_t bit = (data[sy * rowBytes + sx / 8] >> (7 - (sx % 8))) & 1;
                if (inClip(x + sx, y + sy, clipRect)) plot(x + sx, y + sy, bit);
            }
    }

    uint8_t pixel(int x, int y) const {
        return this->framebuffer[(size_t)y * this->nativeWidth + x];
    }

    void reset() {
        this->ops.clear();
        std::fill(this->framebuffer.begin(), this->framebuffer.end(), 0);
    }

    std::vector<Op> ops;
    std::vector<uint8_t> framebuffer;

private:
    bool inClip(int px, int py, focus::Rect clip) const {
        if (px < 0 || py < 0 || px >= this->nativeWidth || py >= this->nativeHeight) return false;
        if (clip.size.width <= 0 || clip.size.height <= 0) return true;
        return px >= clip.origin.x && px < clip.origin.x + clip.size.width &&
               py >= clip.origin.y && py < clip.origin.y + clip.size.height;
    }
    void plot(int px, int py, uint8_t v) {
        this->framebuffer[(size_t)py * this->nativeWidth + px] = v;
    }
};
