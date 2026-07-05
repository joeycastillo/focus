#include "InputTask.hpp"
#include "Application.hpp"
#include "Focus.hpp"
#include <Arduino.h>
#include <cstdio>
#include <cstdlib>

using namespace focus;

// PyGamer input pins
static constexpr uint8_t kJoystickXPin = A11;
static constexpr uint8_t kJoystickYPin = A10;
static constexpr uint8_t kButtonLatchPin = 50;
static constexpr uint8_t kButtonClockPin = 48;
static constexpr uint8_t kButtonDataPin = 49;

// bitmasks for buttons, read from 74HC165 shift register
static constexpr uint8_t kMaskB = 0x80;
static constexpr uint8_t kMaskA = 0x40;
// Start = 0x20, Select = 0x10: unmapped (we don't use them).

// Joystick thresholds on the 10-bit ADC (0..1023, center ~512). A push
// past kTrigger emits one DIRECTION event; the stick must return within
// kRelease of center before another can fire.
static constexpr int kCenter = 512;
static constexpr int kTrigger = 200;
static constexpr int kRelease = 100;

// 10 ms between to debounces tactile switches.
static constexpr uint32_t kPollIntervalMs = 10;

InputTask::InputTask() {
    pinMode(kButtonLatchPin, OUTPUT);
    pinMode(kButtonClockPin, OUTPUT);
    pinMode(kButtonDataPin, INPUT);
    digitalWrite(kButtonLatchPin, HIGH);
    digitalWrite(kButtonClockPin, LOW);
}

uint8_t InputTask::readButtons() {
    // Snapshot the 74HC165 inputs (latch pulse), then clock out 8 bits.
    digitalWrite(kButtonLatchPin, LOW);
    delayMicroseconds(1);
    digitalWrite(kButtonLatchPin, HIGH);
    delayMicroseconds(1);

    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 1;
        result |= digitalRead(kButtonDataPin);
        digitalWrite(kButtonClockPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(kButtonClockPin, LOW);
        delayMicroseconds(1);
    }

    return result;
}

bool InputTask::run(std::shared_ptr<Application> application) {
    uint32_t now = millis();
    if (now - lastPollMs < kPollIntervalMs) return false;
    lastPollMs = now;

    int dx = analogRead(kJoystickXPin) - kCenter;
    int dy = analogRead(kJoystickYPin) - kCenter;

    if (joystickArmed) {
        if (abs(dx) > kTrigger || abs(dy) > kTrigger) {
            int32_t event;
            if (abs(dx) >= abs(dy)) {
                event = (dx > 0) ? FOCUS_EVENT_DIRECTION_RIGHT
                                 : FOCUS_EVENT_DIRECTION_LEFT;
            } else {
                event = (dy > 0) ? FOCUS_EVENT_DIRECTION_DOWN
                                 : FOCUS_EVENT_DIRECTION_UP;
            }
            application->generateEvent(event, 0);
            joystickArmed = false;
        }
    } else if (abs(dx) < kRelease && abs(dy) < kRelease) {
        joystickArmed = true;
    }

    uint8_t buttons = readButtons();
    uint8_t pressed = buttons & (uint8_t)~prevButtons;
    prevButtons = buttons;

    if (pressed & kMaskA) {
        application->generateEvent(FOCUS_EVENT_SELECT, 0);
    }
    if (pressed & kMaskB) {
        application->generateEvent(FOCUS_EVENT_BACK, 0);
    }

    return false;  // long-lived task, never removed
}
