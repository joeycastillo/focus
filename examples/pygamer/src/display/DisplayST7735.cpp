#include "DisplayST7735.hpp"
#include <algorithm>
#include <cstring>

using focus::Rect;

// PyGamer TFT pins
static constexpr int8_t kTFTCS = 44;
static constexpr int8_t kTFTDC = 45;
static constexpr int8_t kTFTRST = 46;
static constexpr int8_t kTFTBacklight = 47;

DisplayST7735::DisplayST7735() : tft(&SPI1, kTFTCS, kTFTDC, kTFTRST), pixels(kWidth * kHeight, 0x0000) {
    this->displayMode = focus::DisplayMode::RGB565;
    this->nativeWidth = kWidth;
    this->nativeHeight = kHeight;

    pinMode(kTFTBacklight, OUTPUT);
    digitalWrite(kTFTBacklight, HIGH);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);         // landscape; hardware owns rotation, Focus stays 0
    tft.setSPISpeed(24000000);
    tft.startWrite();
    tft.setAddrWindow(0, 0, kWidth, kHeight);
    for (uint32_t i = 0; i < (uint32_t)kWidth * kHeight; i++) tft.SPI_WRITE16(0x0000);
    tft.endWrite();
}

// --- Buffer writers: logic cribbed from Libros' SDLRGBDisplay ---

void DisplayST7735::fillRect(int x, int y, int w, int h, uint16_t color, Rect clipRect) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(kWidth, x + w);
    int y1 = std::min(kHeight, y + h);
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
            memset(reinterpret_cast<uint8_t*>(pixels.data() + row * kWidth + x0),
                   byte, spanW * 2);
        }
    } else {
        // Fill first row with a loop, then memcpy into remaining rows.
        uint16_t* firstRow = pixels.data() + y0 * kWidth + x0;
        for (int col = 0; col < spanW; col++) firstRow[col] = color;
        for (int row = y0 + 1; row < y1; row++) {
            memcpy(pixels.data() + row * kWidth + x0, firstRow, spanW * sizeof(uint16_t));
        }
    }
}

void DisplayST7735::blitOpaque(int x, int y, int w, int h,
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
        if (dy < 0 || dy >= kHeight) continue;

        const uint16_t* srcPixel = reinterpret_cast<const uint16_t*>(data + sy * rowBytes) + sx0;
        int dx = x + sx0;
        int count = sx1 - sx0;

        if (dx < 0) { srcPixel += (-dx); count += dx; dx = 0; }
        if (dx + count > kWidth) count = kWidth - dx;
        if (count <= 0) continue;

        memcpy(pixels.data() + dy * kWidth + dx, srcPixel, count * sizeof(uint16_t));
    }
}

void DisplayST7735::blitMasked(int x, int y, int w, int h, uint16_t color,
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
        if (dy < 0 || dy >= kHeight) continue;
        for (int sx = sx0; sx < sx1; sx++) {
            int dx = x + sx;
            if (dx < 0 || dx >= kWidth) continue;
            bool maskBit = mask[sy * rowBytes + (sx >> 3)] & (0x80 >> (sx & 7));
            if (maskBit) {
                pixels[dy * kWidth + dx] = color;
            }
        }
    }
}

void DisplayST7735::flush(Rect rect) {
    int x0 = 0, y0 = 0, x1 = kWidth, y1 = kHeight;
    if (rect.size.width > 0 && rect.size.height > 0) {
        x0 = std::max(0, rect.origin.x);
        y0 = std::max(0, rect.origin.y);
        x1 = std::min(kWidth, rect.origin.x + rect.size.width);
        y1 = std::min(kHeight, rect.origin.y + rect.size.height);
        if (x0 >= x1 || y0 >= y1) return;
    }
    int w = x1 - x0;

    tft.startWrite();
    tft.setAddrWindow(x0, y0, w, y1 - y0);
    for (int row = y0; row < y1; row++) {
        const uint16_t* srcRow = &pixels[row * kWidth + x0];
        for (int col = 0; col < w; col++) tft.SPI_WRITE16(srcRow[col]);
    }
    tft.endWrite();
}
