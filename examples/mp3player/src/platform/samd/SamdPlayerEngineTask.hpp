#pragma once

#include "Task.hpp"
#include "NotificationCenter.hpp"
#include "app/PlayerNotifications.hpp"
#include "platform/samd/SamdPlayerEngine.hpp"
#include <memory>

/// Pumps the Helix decoder each pass and announces end of track.
class SamdPlayerEngineTask : public focus::Task {
public:
    explicit SamdPlayerEngineTask(std::shared_ptr<SamdPlayerEngine> engine)
        : engine(engine) {}

    bool run(std::shared_ptr<focus::Application> application) override {
        engine->serviceDecoder();
        if (engine->consumeFinishedFlag()) {
            focus::NotificationCenter::shared()->post(player::kPlaybackEnded);
        }
        return false;
    }

private:
    std::shared_ptr<SamdPlayerEngine> engine;
};
