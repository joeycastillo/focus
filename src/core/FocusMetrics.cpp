/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
 */

#include "FocusMetrics.hpp"

static FocusMetrics sActiveMetrics;
static bool sMetricsConfigured = false;

// Compact: 128x64 LCD, Blit16 font (3x5 visible, gh=7)
// Verified against Evan Kahn's go board device.
const FocusMetrics FocusMetrics::Compact = {
    .navBarHeight = 14,
    .navBarPadding = 3,
    .navBarButtonWidth = 30,
    .tabBarHeight = 13,
    .arrowThickness = 11,
    .footerThickness = 11,
    .footerGap = 2,
    .footerButtonWidth = 35,
};

// Standard: 320x240 TFT, 5x8 font (gh=8)
const FocusMetrics FocusMetrics::Standard = {
    .navBarHeight = 25,
    .navBarPadding = 8,
    .navBarButtonWidth = 50,
    .tabBarHeight = 24,
    .arrowThickness = 16,
    .footerThickness = 16,
    .footerGap = 4,
    .footerButtonWidth = 50,
};

// Large: 480x800+ display, 24-32px system font
const FocusMetrics FocusMetrics::Large = {
    .navBarHeight = 64,
    .navBarPadding = 8,
    .navBarButtonWidth = 80,
    .tabBarHeight = 48,
    .arrowThickness = 36,
    .footerThickness = 48,
    .footerGap = 8,
    .footerButtonWidth = 100,
};

void FocusMetrics::set(const FocusMetrics& metrics) {
    sActiveMetrics = metrics;
    sMetricsConfigured = true;
}

const FocusMetrics& FocusMetrics::get() {
    if (!sMetricsConfigured) return Standard;
    return sActiveMetrics;
}
