#pragma once

#include "Task.hpp"
#include <memory>

class DisplayST7735;

/// Screen Refresh Task: when the window has a dirty region, draw the view
/// tree into the DisplayST7735 framebuffer, flush region to the panel,
/// then clear the dirty flag.
class RefreshTask : public focus::Task {
public:
    explicit RefreshTask(std::shared_ptr<DisplayST7735> display);
    bool run(std::shared_ptr<focus::Application> application) override;

private:
    std::shared_ptr<DisplayST7735> display;
};
