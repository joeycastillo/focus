#pragma once

#include "Task.hpp"
#include <memory>
#include <stdint.h>

/// Polls the PyGamer's analog joystick and buttons, injecting Focus events:
///  * Joystick -> FOCUS_EVENT_DIRECTION_* (one event per push)
///  * A -> FOCUS_EVENT_SELECT
///  * B -> FOCUS_EVENT_BACK
class InputTask : public focus::Task {
public:
    InputTask();
    bool run(std::shared_ptr<focus::Application> application) override;

private:
    uint8_t readButtons();

    uint32_t lastPollMs = 0;
    uint8_t prevButtons = 0;
    bool joystickArmed = true;
};
