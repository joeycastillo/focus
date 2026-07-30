#include "test_harness.hpp"
#include "PackedFontGlyphProvider.hpp"

#include <fstream>
#include <iterator>
#include <vector>
#include <cstdint>

using namespace focus;

static const char* SPLEEN = FONTS_DIR "spleen-8x16.bdp";

static std::vector<uint8_t> readAllBytes(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}

TEST(packed_font_file_and_memory_are_equivalent) {
    PackedFontGlyphProvider fileProvider(SPLEEN);
    ASSERT_TRUE(fileProvider.isValid());

    std::vector<uint8_t> bytes = readAllBytes(SPLEEN);
    ASSERT_GT(bytes.size(), (size_t)16);

    auto mem = PackedFontGlyphProvider::fromMemory(bytes.data(), bytes.size());
    ASSERT_TRUE(mem != nullptr);
    ASSERT_TRUE(mem->isValid());

    ASSERT_STREQ(fileProvider.getTitle(), mem->getTitle());
    ASSERT_EQ(fileProvider.getGlyphCount(), mem->getGlyphCount());
    ASSERT_EQ(fileProvider.getPointSize(), mem->getPointSize());
    ASSERT_EQ(fileProvider.getGlyphRowCount(), mem->getGlyphRowCount());

    for (UNICODE_CODEPOINT cp = 32; cp < 127; ++cp) {
        ASSERT_EQ(fileProvider.hasGlyph(cp), mem->hasGlyph(cp));
        if (!fileProvider.hasGlyph(cp)) continue;
        GlyphMetrics a = fileProvider.metricsForCodepoint(cp);
        GlyphMetrics b = mem->metricsForCodepoint(cp);
        ASSERT_EQ(a.advance, b.advance);
        ASSERT_EQ(a.bitmapWidth, b.bitmapWidth);
        ASSERT_EQ(a.height, b.height);
    }
}

TEST(packed_font_from_memory_rejects_garbage) {
    const uint8_t garbage[16] = {0};
    auto mem = PackedFontGlyphProvider::fromMemory(garbage, sizeof(garbage));
    ASSERT_TRUE(mem == nullptr);
}
