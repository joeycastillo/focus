/*
 * Tests for GrayscaleColor and RGB565Color conversions.
 */

#include "test_harness.hpp"
#include "Color.hpp"

using namespace focus;

// --- GrayscaleColor ---

TEST(grayscale_black) {
    ASSERT_EQ(GrayscaleColor::Black(), (uint16_t)0x0000);
}

TEST(grayscale_white) {
    ASSERT_EQ(GrayscaleColor::White(), (uint16_t)0xFFFF);
}

TEST(grayscale_dark_gray) {
    ASSERT_EQ(GrayscaleColor::DarkGray(), (uint16_t)0x5555);
}

TEST(grayscale_light_gray) {
    ASSERT_EQ(GrayscaleColor::LightGray(), (uint16_t)0xAAAA);
}

TEST(grayscale_2bit_extraction) {
    // 2-bit: color >> 14 gives 0..3
    ASSERT_EQ(GrayscaleColor::Black() >> 14, 0);
    ASSERT_EQ(GrayscaleColor::DarkGray() >> 14, 1);
    ASSERT_EQ(GrayscaleColor::LightGray() >> 14, 2);
    ASSERT_EQ(GrayscaleColor::White() >> 14, 3);
}

// --- RGB565Color ---

TEST(rgb565_black) {
    ASSERT_EQ(RGB565Color::Black(), (uint16_t)0x0000);
}

TEST(rgb565_white) {
    ASSERT_EQ(RGB565Color::White(), (uint16_t)0xFFFF);
}

TEST(rgb565_red) {
    // Red = 5 bits max red, 0 green, 0 blue = 0xF800
    ASSERT_EQ(RGB565Color::Red(), (uint16_t)0xF800);
}

TEST(rgb565_green) {
    // Green = 0 red, 6 bits max green, 0 blue = 0x07E0
    ASSERT_EQ(RGB565Color::Green(), (uint16_t)0x07E0);
}

TEST(rgb565_blue) {
    // Blue = 0 red, 0 green, 5 bits max blue = 0x001F
    ASSERT_EQ(RGB565Color::Blue(), (uint16_t)0x001F);
}

TEST(rgb565_from_rgb_black) {
    ASSERT_EQ(RGB565Color::fromRGB(0, 0, 0), (uint16_t)0x0000);
}

TEST(rgb565_from_rgb_white) {
    ASSERT_EQ(RGB565Color::fromRGB(255, 255, 255), (uint16_t)0xFFFF);
}

TEST(rgb565_from_rgb_pure_red) {
    ASSERT_EQ(RGB565Color::fromRGB(255, 0, 0), RGB565Color::Red());
}

TEST(rgb565_from_rgb_pure_green) {
    ASSERT_EQ(RGB565Color::fromRGB(0, 255, 0), RGB565Color::Green());
}

TEST(rgb565_from_rgb_pure_blue) {
    ASSERT_EQ(RGB565Color::fromRGB(0, 0, 255), RGB565Color::Blue());
}

TEST(rgb565_from_grayscale_black) {
    ASSERT_EQ(RGB565Color::fromGrayscale(0), (uint16_t)0x0000);
}

TEST(rgb565_from_grayscale_white) {
    ASSERT_EQ(RGB565Color::fromGrayscale(255), (uint16_t)0xFFFF);
}

TEST(rgb565_from_grayscale_mid) {
    // 128 → R=128&0xF8=128, G=128&0xFC=128, B=128>>3=16
    // R: (128 << 8) = 0x8000, G: (128 << 3) = 0x0400, B: 16 = 0x0010
    // = 0x8410
    ASSERT_EQ(RGB565Color::fromGrayscale(128), RGB565Color::Gray());
}

TEST(rgb565_secondary_colors) {
    ASSERT_EQ(RGB565Color::Yellow(), (uint16_t)0xFFE0);   // R+G
    ASSERT_EQ(RGB565Color::Cyan(), (uint16_t)0x07FF);     // G+B
    ASSERT_EQ(RGB565Color::Magenta(), (uint16_t)0xF81F);  // R+B
}
