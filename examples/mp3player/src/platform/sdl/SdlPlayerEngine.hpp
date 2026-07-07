#pragma once

#include "app/PlayerEngine.hpp"
#include <SDL.h>
#include <atomic>
#include "platform/sdl/dr_mp3.h"

/// Desktop engine: dr_mp3 decoding inside the SDL audio callback.
class SdlPlayerEngine : public PlayerEngine {
public:
    explicit SdlPlayerEngine(std::string musicDir);
    ~SdlPlayerEngine();

    std::vector<TrackInfo> scanLibrary() override;
    bool play(const TrackInfo& track) override;
    void pause() override;
    void resume() override;
    void stop() override;
    bool isPlaying() const override;
    uint32_t elapsedMs() const override;
    std::string libraryLocationHint() const override;

    /// One-shot read of the audio thread's end-of-track flag.
    bool consumeFinishedFlag();

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);

    std::string musicDir;
    SDL_AudioDeviceID device = 0;
    drmp3 decoder{};
    bool decoderOpen = false;
    bool playing = false;
    bool paused = false;
    uint64_t framesPlayed = 0;   ///< Guarded by SDL_LockAudioDevice.
    std::atomic<bool> finished{false};
};
