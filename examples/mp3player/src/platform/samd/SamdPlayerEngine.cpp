#include "platform/samd/SamdPlayerEngine.hpp"
#include "platform/samd/Mp3StreamInfo.hpp"
#include <Arduino.h>
#include <algorithm>
#include <cstring>

SamdPlayerEngine* SamdPlayerEngine::instance = nullptr;

// Read at most this much per buffer callback so decode stays responsive.
static constexpr int kMaxReadBytes = 768;
// Drain time after the file runs out, before declaring the track finished.
static constexpr uint32_t kDrainMs = 250;

SamdPlayerEngine::SamdPlayerEngine(uint8_t sdCsPin, uint8_t speakerEnablePin)
    : sdCs(sdCsPin), speakerEnable(speakerEnablePin) {
    instance = this;
    pinMode(speakerEnable, OUTPUT);
    digitalWrite(speakerEnable, LOW);
    analogWriteResolution(12);
    analogWrite(A0, 2048);  // enable the DAC now (first enable stalls 10 ms) and park at mid-rail
}

std::vector<TrackInfo> SamdPlayerEngine::scanLibrary() {
    std::vector<TrackInfo> tracks;

    // Close the playback file before the remount pulls the volume away.
    stop();

    // Remount so a freshly inserted card is picked up by Rescan.
    if (mounted) {
        sd.end();
        mounted = false;
    }
    if (!sd.begin(SdSpiConfig(sdCs, SHARED_SPI))) {
        return tracks;
    }
    mounted = true;

    FsFile dir = sd.open("/music");
    if (!dir || !dir.isDir()) return tracks;

    FsFile entry;
    while (entry.openNext(&dir, O_RDONLY)) {
        char name[96];
        entry.getName(name, sizeof(name));
        size_t len = strlen(name);
        bool isMp3 = len > 4 && strcasecmp(name + len - 4, ".mp3") == 0 && name[0] != '.';
        if (!entry.isDir() && isMp3) {
            TrackInfo track;
            track.path = std::string("/music/") + name;
            track.title = std::string(name, len - 4);
            track.durationMs = 0;
            Mp3StreamInfo info;
            if (readMp3StreamInfo(entry, info)) {
                uint64_t audioBytes = (uint64_t)entry.fileSize() - info.audioOffset;
                track.durationMs = (uint32_t)(audioBytes * 8ULL / info.bitrateKbps);
            }
            tracks.push_back(std::move(track));
        }
        entry.close();
    }
    dir.close();

    std::sort(tracks.begin(), tracks.end(),
              [](const TrackInfo& a, const TrackInfo& b) { return a.title < b.title; });
    return tracks;
}

bool SamdPlayerEngine::play(const TrackInfo& track) {
    stop();
    if (!mounted) return false;

    file = sd.open(track.path.c_str(), O_RDONLY);
    if (!file) return false;

    Mp3StreamInfo info;
    if (readMp3StreamInfo(file, info)) {
        sampleRate = info.sampleRate;
        file.seekSet(info.audioOffset);  // skip the ID3v2 tag; decode from audio data
    }

    // begin() clears both callbacks, so they must be registered after it.
    if (!begun) {
        player.begin();
        player.setBufferCallback(&SamdPlayerEngine::bufferCallbackThunk);
        player.setSampleReadyCallback(&SamdPlayerEngine::sampleCallbackThunk);
        begun = true;
    }

    samplesPlayed = 0;
    fileExhausted = false;
    finishedFlag = false;
    exhaustedAtMs = 0;
    digitalWrite(speakerEnable, HIGH);
    player.play();
    if (info.sampleRate) {
        // Retune the output timer; the library only retunes vs its compile-time default.
        MP3_TC->COUNT16.CC[0].reg = (uint16_t)((SystemCoreClock >> 2) / info.sampleRate);
        while (MP3_TC->COUNT16.SYNCBUSY.bit.CC0) {}
    }
    playing = true;
    paused = false;
    return true;
}

void SamdPlayerEngine::pause() {
    if (!playing) return;
    player.pause();
    paused = true;
}

void SamdPlayerEngine::resume() {
    if (!playing) return;
    player.resume();
    paused = false;
}

void SamdPlayerEngine::stop() {
    if (playing) player.pause();
    if (file) file.close();
    digitalWrite(speakerEnable, LOW);
    playing = false;
    paused = false;
}

bool SamdPlayerEngine::isPlaying() const { return playing && !paused; }

uint32_t SamdPlayerEngine::elapsedMs() const {
    return (uint32_t)((uint64_t)samplesPlayed * 1000ULL / sampleRate);
}

std::string SamdPlayerEngine::libraryLocationHint() const {
    return "/music on the SD card";
}

void SamdPlayerEngine::serviceDecoder() {
    if (!playing || paused) return;
    player.tick();
    if (fileExhausted) {
        uint32_t now = millis();
        if (exhaustedAtMs == 0) {
            exhaustedAtMs = now;
        } else if (now - exhaustedAtMs > kDrainMs) {
            stop();
            finishedFlag = true;
        }
    }
}

bool SamdPlayerEngine::consumeFinishedFlag() {
    bool was = finishedFlag;
    finishedFlag = false;
    return was;
}

int SamdPlayerEngine::bufferCallbackThunk(uint8_t* dst, int len) {
    return instance ? instance->fillBuffer(dst, len) : 0;
}

void SamdPlayerEngine::sampleCallbackThunk(int16_t left, int16_t right) {
    if (instance) instance->emitSample(left);
}

int SamdPlayerEngine::fillBuffer(uint8_t* dst, int len) {
    int n = file ? file.read(dst, std::min(len, kMaxReadBytes)) : 0;
    if (n <= 0) {
        fileExhausted = true;
        return 0;
    }
    return n;
}

void SamdPlayerEngine::emitSample(int16_t left) {
    samplesPlayed = samplesPlayed + 1;
    // Map signed 16-bit PCM onto the 12-bit DAC (mono speaker on A0).
    analogWrite(A0, ((int32_t)left + 32768) >> 4);
}
