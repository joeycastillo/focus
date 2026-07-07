#include "platform/samd/Mp3StreamInfo.hpp"

static const uint16_t kBitrateKbps[] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
static const uint16_t kSampleRates[] = {44100, 48000, 32000};

bool readMp3StreamInfo(FsFile& file, Mp3StreamInfo& out) {
    out = {0, 0, 0};
    file.seekSet(0);

    // Skip an ID3v2 tag if present (10-byte header, syncsafe size).
    uint8_t header[10];
    if (file.read(header, 10) == 10 &&
        header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
        uint32_t tagSize = ((uint32_t)(header[6] & 0x7F) << 21) |
                           ((uint32_t)(header[7] & 0x7F) << 14) |
                           ((uint32_t)(header[8] & 0x7F) << 7) |
                           (uint32_t)(header[9] & 0x7F);
        out.audioOffset = 10 + tagSize;
    }
    file.seekSet(out.audioOffset);

    // Scan a window for the first MPEG-1 Layer III frame sync.
    uint8_t buf[2048];
    int n = file.read(buf, sizeof(buf));
    for (int i = 0; i + 3 < n; i++) {
        if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) continue;
        bool mpeg1 = ((buf[i + 1] >> 3) & 0x03) == 0x03;
        bool layer3 = ((buf[i + 1] >> 1) & 0x03) == 0x01;
        uint8_t bitrateIndex = (buf[i + 2] >> 4) & 0x0F;
        uint8_t sampleRateIndex = (buf[i + 2] >> 2) & 0x03;
        if (!mpeg1 || !layer3 || bitrateIndex == 0 || bitrateIndex == 15 ||
            sampleRateIndex == 3) continue;
        out.bitrateKbps = kBitrateKbps[bitrateIndex];
        out.sampleRate = kSampleRates[sampleRateIndex];
        break;
    }

    file.seekSet(0);
    return out.bitrateKbps != 0;
}
