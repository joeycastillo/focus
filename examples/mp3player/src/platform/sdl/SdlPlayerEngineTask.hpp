#pragma once

#include "Task.hpp"
#include "NotificationCenter.hpp"
#include "app/PlayerNotifications.hpp"
#include "platform/sdl/SdlPlayerEngine.hpp"
#include <memory>

/// Relays the audio thread's end-of-track flag onto the run loop.
class SdlPlayerEngineTask : public focus::Task {
public:
    explicit SdlPlayerEngineTask(std::shared_ptr<SdlPlayerEngine> engine)
        : engine(engine) {}

    bool run(std::shared_ptr<focus::Application> application) override {
        if (engine->consumeFinishedFlag()) {
            engine->stop();
            focus::NotificationCenter::shared()->post(player::kPlaybackEnded);
        }
        return false;
    }

private:
    std::shared_ptr<SdlPlayerEngine> engine;
};
