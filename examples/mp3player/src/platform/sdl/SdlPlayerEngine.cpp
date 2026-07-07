#include "platform/sdl/SdlPlayerEngine.hpp"
#define DR_MP3_IMPLEMENTATION
#include "platform/sdl/dr_mp3.h"
#include <algorithm>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

SdlPlayerEngine::SdlPlayerEngine(std::string musicDir) : musicDir(std::move(musicDir)) {}

SdlPlayerEngine::~SdlPlayerEngine() { stop(); }

std::vector<TrackInfo> SdlPlayerEngine::scanLibrary() {
    std::vector<TrackInfo> tracks;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(musicDir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        if (!filename.empty() && filename[0] == '.') continue;  // skip hidden files
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".mp3") continue;

        TrackInfo track;
        track.path = entry.path().string();
        track.title = entry.path().stem().string();
        track.durationMs = 0;
        drmp3 probe;
        if (drmp3_init_file(&probe, track.path.c_str(), NULL)) {
            drmp3_uint64 frames = drmp3_get_pcm_frame_count(&probe);
            if (probe.sampleRate > 0) {
                track.durationMs = (uint32_t)(frames * 1000 / probe.sampleRate);
            }
            drmp3_uninit(&probe);
        }
        tracks.push_back(std::move(track));
    }
    std::sort(tracks.begin(), tracks.end(),
              [](const TrackInfo& a, const TrackInfo& b) { return a.title < b.title; });
    return tracks;
}

std::string SdlPlayerEngine::libraryLocationHint() const { return musicDir; }
bool SdlPlayerEngine::isPlaying() const { return playing && !paused; }
bool SdlPlayerEngine::consumeFinishedFlag() { return finished.exchange(false); }

bool SdlPlayerEngine::play(const TrackInfo& track) {
    stop();

    if (!drmp3_init_file(&decoder, track.path.c_str(), NULL)) {
        return false;
    }
    decoderOpen = true;

    // Open a device matching the decoded stream's rate and channels
    SDL_AudioSpec want{};
    want.freq = (int)decoder.sampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = (Uint8)decoder.channels;
    want.samples = 1024;
    want.callback = &SdlPlayerEngine::audioCallback;
    want.userdata = this;

    device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (device == 0) {
        drmp3_uninit(&decoder);
        decoderOpen = false;
        return false;
    }

    framesPlayed = 0;
    finished = false;
    playing = true;
    paused = false;
    SDL_PauseAudioDevice(device, 0);
    return true;
}

void SdlPlayerEngine::pause() {
    if (device) SDL_PauseAudioDevice(device, 1);
    paused = true;
}

void SdlPlayerEngine::resume() {
    if (device) SDL_PauseAudioDevice(device, 0);
    paused = false;
}

void SdlPlayerEngine::stop() {
    // Close the device to stop the callback before freeing the decoder
    if (device) {
        SDL_CloseAudioDevice(device);
        device = 0;
    }
    if (decoderOpen) {
        drmp3_uninit(&decoder);
        decoderOpen = false;
    }
    playing = false;
    paused = false;
}

uint32_t SdlPlayerEngine::elapsedMs() const {
    if (!device || decoder.sampleRate == 0) return 0;
    SDL_LockAudioDevice(device);
    uint64_t frames = framesPlayed;
    SDL_UnlockAudioDevice(device);
    return (uint32_t)(frames * 1000 / decoder.sampleRate);
}

void SdlPlayerEngine::audioCallback(void* userdata, Uint8* stream, int len) {
    // Runs on the SDL audio thread: decode MP3 frames straight into SDL's buffer
    auto* self = static_cast<SdlPlayerEngine*>(userdata);
    int bytesPerFrame = (int)(self->decoder.channels * sizeof(drmp3_int16));
    drmp3_uint64 framesWanted = (drmp3_uint64)(len / bytesPerFrame);
    drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(
        &self->decoder, framesWanted, (drmp3_int16*)stream);
    self->framesPlayed += framesRead;
    if (framesRead < framesWanted) {
        // Underrun? Fill the rest of the buffer with silence.
        memset(stream + framesRead * bytesPerFrame, 0,
               (size_t)((framesWanted - framesRead) * bytesPerFrame));
        if (framesRead == 0) self->finished = true;   // fully drained; track is over
    }
}
