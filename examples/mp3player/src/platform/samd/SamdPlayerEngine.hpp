#pragma once

#include "app/PlayerEngine.hpp"
#include <Adafruit_MP3.h>
#include <SdFat.h>
#include <cstdint>

/// SAMD51 engine: Helix decoding via Adafruit_MP3, files from SD, samples
/// out the A0 DAC from the library's sample-rate timer interrupt.
class SamdPlayerEngine : public PlayerEngine {
public:
    SamdPlayerEngine(uint8_t sdCsPin, uint8_t speakerEnablePin);

    std::vector<TrackInfo> scanLibrary() override;
    bool play(const TrackInfo& track) override;
    void pause() override;
    void resume() override;
    void stop() override;
    bool isPlaying() const override;
    uint32_t elapsedMs() const override;
    std::string libraryLocationHint() const override;

    /// Pump the decoder and watch for end of stream. Called by the engine task.
    void serviceDecoder();

    /// One-shot read of the end-of-track flag set by serviceDecoder().
    bool consumeFinishedFlag();

private:
    static int bufferCallbackThunk(uint8_t* dst, int len);
    static void sampleCallbackThunk(int16_t left, int16_t right);
    int fillBuffer(uint8_t* dst, int len);
    void emitSample(int16_t left);

    static SamdPlayerEngine* instance;   ///< For the C-style library callbacks.

    Adafruit_MP3 player;
    SdFs sd;
    FsFile file;
    uint8_t sdCs;
    uint8_t speakerEnable;
    bool begun = false;
    bool mounted = false;
    bool playing = false;
    bool paused = false;
    bool finishedFlag = false;
    volatile bool fileExhausted = false;
    volatile uint32_t samplesPlayed = 0;
    uint32_t sampleRate = 44100;
    uint32_t exhaustedAtMs = 0;
};
