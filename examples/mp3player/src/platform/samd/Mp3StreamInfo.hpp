#pragma once

#include <SdFat.h>
#include <cstdint>

/// CBR stream facts read from the first MPEG-1 Layer III frame header.
struct Mp3StreamInfo {
    uint32_t bitrateKbps;
    uint32_t sampleRate;
    uint32_t audioOffset;   ///< Bytes to skip (ID3v2 tag) before audio data.
};

/// Reads the header without disturbing the file's read position contract:
/// leaves the file positioned at 0. Returns false if no frame sync is found.
bool readMp3StreamInfo(FsFile& file, Mp3StreamInfo& out);
