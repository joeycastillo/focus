#pragma once

#include "app/TrackInfo.hpp"
#include <vector>
#include <string>
#include <cstdint>

/// Abstract playback backend. UI code talks only to this interface.
/// End of track is announced by posting player::kPlaybackEnded notification.
class PlayerEngine {
public:
    virtual ~PlayerEngine() {}

    /// (Re)mount storage if needed, then enumerate available tracks.
    virtual std::vector<TrackInfo> scanLibrary() = 0;

    virtual bool play(const TrackInfo& track) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual uint32_t elapsedMs() const = 0;

    /// Where the user should put music, e.g. "/music on the SD card".
    virtual std::string libraryLocationHint() const = 0;
};
