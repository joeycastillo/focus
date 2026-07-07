#pragma once

#include "Task.hpp"
#include "platform/sdl/ScriptCommandProcessor.hpp"
#include <memory>

class SDLDisplay;

/// Input mode selected by the --input flag.
enum class InputMode { Touch, DPad };

/// Translates SDL events into Focus events: mouse -> touch (Touch mode),
/// arrows/enter/escape -> direction/select/back (DPad mode), S -> screenshot.
/// With scriptMode, also executes stdin commands via ScriptCommandProcessor.
class SDLInputTask : public focus::Task, public ScriptCommandDelegate {
public:
    SDLInputTask(std::shared_ptr<SDLDisplay> display, InputMode mode, int scale,
                 bool scriptMode);
    bool run(std::shared_ptr<focus::Application> application) override;

    std::string handleScreenshot(const std::string& path) override;
    std::string handleQuit() override;

private:
    bool hasStdinData();
    void processNextCommand();

    std::shared_ptr<SDLDisplay> display;
    InputMode mode;
    int scale;
    bool scriptMode;
    int screenshotCounter = 0;
    std::unique_ptr<ScriptCommandProcessor> processor;
};
