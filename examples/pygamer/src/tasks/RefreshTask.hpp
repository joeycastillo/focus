#pragma once

#include "Task.hpp"
#include "Display.hpp"
#include <memory>

/// Screen Refresh Task: when the window has a dirty region, draw the view
/// tree into the display, flush the region to the panel, then clear the
/// dirty flag.
class RefreshTask : public focus::Task {
public:
    explicit RefreshTask(std::shared_ptr<focus::Display> display);
    bool run(std::shared_ptr<focus::Application> application) override;

private:
    std::shared_ptr<focus::Display> display;
};
