#include "platform/samd/pyportal/TouchInputTask.hpp"
#include "Application.hpp"
#include "Focus.hpp"
#include <Arduino.h>

using namespace focus;

// PyPortal touchscreen pins (analog resistive)
static constexpr uint8_t kXPlus = A5;
static constexpr uint8_t kYPlus = A4;
static constexpr uint8_t kXMinus = A7;
static constexpr uint8_t kYMinus = A6;
static constexpr uint16_t kPlateOhms = 300;

// Pressure window that counts as a touch. p.z reads as resistance, so LOWER = firmer
// press: the floor is kept low so firm presses aren't rejected.
static constexpr int kMinPressure = 10;
static constexpr int kMaxPressure = 1000;

// Raw ADC extents mapped to screen pixels. Starting values from Adafruit's
// examples; tune against the real panel (enable kDebugRaw to print raws).
static constexpr int kRawXMin = 150;
static constexpr int kRawXMax = 850;
static constexpr int kRawYMin = 150;
static constexpr int kRawYMax = 850;
static constexpr bool kDebugRaw = false;

static constexpr int kScreenWidth = 320;
static constexpr int kScreenHeight = 240;
static constexpr uint32_t kPollIntervalMs = 10;

TouchInputTask::TouchInputTask() : ts(kXPlus, kYPlus, kXMinus, kYMinus, kPlateOhms) {}

bool TouchInputTask::run(std::shared_ptr<Application> application) {
    uint32_t now = millis();
    if (now - lastPollMs < kPollIntervalMs) return false;
    lastPollMs = now;

    TSPoint p = ts.getPoint();
    bool pressed = p.z > kMinPressure && p.z < kMaxPressure;

    if (pressed) {
        if (kDebugRaw) {
            Serial.print("touch raw x="); Serial.print(p.x);
            Serial.print(" y="); Serial.print(p.y);
            Serial.print(" z="); Serial.println(p.z);
        }
        // Landscape (rotation 3): raw Y runs along screen X.
        int x = map(p.y, kRawYMin, kRawYMax, 0, kScreenWidth - 1);
        int y = map(p.x, kRawXMin, kRawXMax, 0, kScreenHeight - 1);
        x = constrain(x, 0, kScreenWidth - 1);
        y = constrain(y, 0, kScreenHeight - 1);
        int32_t packed = (x << 16) | y;

        if (!touching) {
            touching = true;
            application->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packed);
        } else if (abs(x - lastX) > 2 || abs(y - lastY) > 2) {
            application->generateEvent(FOCUS_EVENT_TOUCH_MOVED, packed);
        }
        lastX = x;
        lastY = y;
    } else if (touching) {
        touching = false;
        application->generateEvent(FOCUS_EVENT_TOUCH_UP, (lastX << 16) | lastY);
    }

    return false;
}
