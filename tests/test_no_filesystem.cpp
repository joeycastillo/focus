// Built only as the focus-test-nofs executable, with FOCUS_HAS_FILESYSTEM=0.
// Proves the memory factories work when file loading is compiled out.
#include "Locale.hpp"
#include "PackedFontGlyphProvider.hpp"

#include <fstream>
#include <iterator>
#include <vector>
#include <cstdint>
#include <cstdio>

using namespace focus;

static std::vector<uint8_t> readAllBytes(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}

int main() {
    int failed = 0;

    static const char blob[] = "greeting=Hello\n";
    Locale* loc = Locale::fromMemory("en", (const uint8_t*)blob, sizeof(blob) - 1);
    if (!loc || loc->getString("greeting") != "Hello") {
        fprintf(stderr, "FAIL: Locale::fromMemory\n");
        failed++;
    }

    std::vector<uint8_t> bytes = readAllBytes(FONTS_DIR "spleen-8x16.bdp");
    auto font = PackedFontGlyphProvider::fromMemory(bytes.data(), bytes.size());
    if (!font || !font->isValid() || !font->hasGlyph('A')) {
        fprintf(stderr, "FAIL: PackedFontGlyphProvider::fromMemory\n");
        failed++;
    }

    if (failed == 0) fprintf(stderr, "no-filesystem smoke: OK\n");
    return failed ? 1 : 0;
}
