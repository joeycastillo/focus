#pragma once

#include "Task.hpp"
#include <TouchScreen.h>
#include <cstdint>

/// Polls the 4-wire resistive panel and emits Focus touch events with
/// packed (x << 16) | y screen coordinates.
class TouchInputTask : public focus::Task {
public:
    TouchInputTask();
    bool run(std::shared_ptr<focus::Application> application) override;

private:
    TouchScreen ts;
    bool touching = false;
    int lastX = 0;
    int lastY = 0;
    uint32_t lastPollMs = 0;
};
