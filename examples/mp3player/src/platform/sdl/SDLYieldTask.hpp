#pragma once

#include "Task.hpp"
#include <SDL.h>

/// Paces the cooperative run loop so the emulator doesn't spin a core.
class SDLYieldTask : public focus::Task {
public:
    bool run(std::shared_ptr<focus::Application> application) override {
        SDL_Delay(10);
        return false;
    }
};
