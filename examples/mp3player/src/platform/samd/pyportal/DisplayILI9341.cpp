#include "platform/samd/pyportal/DisplayILI9341.hpp"
#include <Arduino.h>
#include <algorithm>

using focus::Rect;

// PyPortal TFT pins (8-bit parallel bus)
static constexpr int8_t kTFTD0 = 34;
static constexpr int8_t kTFTWR = 26;
static constexpr int8_t kTFTDC = 10;
static constexpr int8_t kTFTCS = 11;
static constexpr int8_t kTFTRST = 24;
static constexpr int8_t kTFTRD = 9;
static constexpr int8_t kTFTBacklight = 25;

DisplayILI9341::DisplayILI9341()
    : tft(tft8bitbus, kTFTD0, kTFTWR, kTFTDC, kTFTCS, kTFTRST, kTFTRD) {
    this->displayMode = focus::DisplayMode::RGB565;
    this->nativeWidth = kWidth;
    this->nativeHeight = kHeight;

    pinMode(kTFTBacklight, OUTPUT);
    digitalWrite(kTFTBacklight, HIGH);

    tft.begin();
    tft.setRotation(3);   // landscape; hardware owns rotation, Focus stays 0
    tft.fillScreen(0x0000);
}

void DisplayILI9341::fillRect(int x, int y, int w, int h, uint16_t color, Rect clipRect) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(kWidth, x + w);
    int y1 = std::min(kHeight, y + h);
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        x0 = std::max(x0, clipRect.origin.x);
        y0 = std::max(y0, clipRect.origin.y);
        x1 = std::min(x1, clipRect.origin.x + clipRect.size.width);
        y1 = std::min(y1, clipRect.origin.y + clipRect.size.height);
    }
    if (x0 >= x1 || y0 >= y1) return;
    // Blocking writes throughout: the library's DMA path paces its WR strobes
    // with TC2, which Adafruit_MP3 claims for the sample clock.
    tft.startWrite();
    tft.setAddrWindow(x0, y0, x1 - x0, y1 - y0);
    for (uint32_t i = (uint32_t)(x1 - x0) * (y1 - y0); i > 0; i--) {
        tft.SPI_WRITE16(color);
    }
    tft.endWrite();
}

void DisplayILI9341::blitOpaque(int x, int y, int w, int h,
                                const uint8_t* data, int rowBytes, Rect clipRect) {
    int sx0 = 0, sy0 = 0, sx1 = w, sy1 = h;
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        sx0 = std::max(0, clipRect.origin.x - x);
        sy0 = std::max(0, clipRect.origin.y - y);
        sx1 = std::min(w, clipRect.origin.x + clipRect.size.width - x);
        sy1 = std::min(h, clipRect.origin.y + clipRect.size.height - y);
        if (sx0 >= sx1 || sy0 >= sy1) return;
    }

    tft.startWrite();
    for (int sy = sy0; sy < sy1; sy++) {
        int dy = y + sy;
        if (dy < 0 || dy >= kHeight) continue;

        const uint16_t* srcPixel = reinterpret_cast<const uint16_t*>(data + sy * rowBytes) + sx0;
        int dx = x + sx0;
        int count = sx1 - sx0;
        if (dx < 0) { srcPixel += (-dx); count += dx; dx = 0; }
        if (dx + count > kWidth) count = kWidth - dx;
        if (count <= 0) continue;

        tft.setAddrWindow(dx, dy, count, 1);
        for (int i = 0; i < count; i++) tft.SPI_WRITE16(srcPixel[i]);
    }
    tft.endWrite();
}

void DisplayILI9341::blitMasked(int x, int y, int w, int h, uint16_t color,
                                const uint8_t* mask, int rowBytes, Rect clipRect) {
    int sx0 = 0, sy0 = 0, sx1 = w, sy1 = h;
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        sx0 = std::max(0, clipRect.origin.x - x);
        sy0 = std::max(0, clipRect.origin.y - y);
        sx1 = std::min(w, clipRect.origin.x + clipRect.size.width - x);
        sy1 = std::min(h, clipRect.origin.y + clipRect.size.height - y);
        if (sx0 >= sx1 || sy0 >= sy1) return;
    }

    tft.startWrite();
    for (int sy = sy0; sy < sy1; sy++) {
        int dy = y + sy;
        if (dy < 0 || dy >= kHeight) continue;

        // Write each run of set mask bits as one address window.
        int runStart = -1;
        for (int sx = sx0; sx <= sx1; sx++) {
            bool bit = false;
            int dx = x + sx;
            if (sx < sx1 && dx >= 0 && dx < kWidth) {
                bit = mask[sy * rowBytes + (sx >> 3)] & (0x80 >> (sx & 7));
            }
            if (bit && runStart < 0) {
                runStart = dx;
            } else if (!bit && runStart >= 0) {
                tft.setAddrWindow(runStart, dy, dx - runStart, 1);
                for (int i = runStart; i < dx; i++) tft.SPI_WRITE16(color);
                runStart = -1;
            }
        }
    }
    tft.endWrite();
}
