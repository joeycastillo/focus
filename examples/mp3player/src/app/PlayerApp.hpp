#pragma once

#include "Application.hpp"
#include "app/PlayerEngine.hpp"
#include <memory>
#include <vector>

namespace focus { class Display; }

/// The example application: owns the engine and playlist order, installs the
/// library screen, and auto-advances when a track ends.
class PlayerApp : public focus::Application {
public:
    PlayerApp(const std::shared_ptr<focus::Window>& window,
              std::shared_ptr<focus::Display> display,
              std::shared_ptr<PlayerEngine> engine);

    void setup() override;

    PlayerEngine& getEngine();
    const std::vector<TrackInfo>& getLibrary() const;
    size_t getCurrentIndex() const;

    void rescanLibrary();
    void playTrackAtIndex(size_t index);
    void playNext();
    void playPrevious();
    void togglePlayPause();

private:
    std::shared_ptr<focus::Display> display;
    std::shared_ptr<PlayerEngine> engine;
    std::vector<TrackInfo> library;
    size_t currentIndex = 0;
};
