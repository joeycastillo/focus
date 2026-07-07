#pragma once

#include <string>
#include <cstdint>

/// One playable track discovered by PlayerEngine::scanLibrary().
struct TrackInfo {
    std::string path;      ///< Platform-native path to the .mp3 file.
    std::string title;     ///< Filename without extension.
    uint32_t durationMs;   ///< Estimated duration; 0 = unknown.
};
