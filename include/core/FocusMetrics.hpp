/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
 */

#pragma once

/// Layout metrics for Focus UI components. Apps select a preset at startup
/// or construct a custom struct. Components read from the active metrics
/// via FocusMetrics::get().
/// @ingroup core
struct FocusMetrics {
    // Navigation bar
    int navBarHeight = 25;
    int navBarPadding = 8;
    int navBarButtonWidth = 50;

    // Tab bar
    int tabBarHeight = 24;

    // PaginatedCollectionView
    int arrowThickness = 16;
    int footerThickness = 16;
    int footerGap = 4;
    int footerButtonWidth = 50;

    /// Named presets validated on real hardware.
    static const FocusMetrics Compact;   ///< 128x64 LCD, tiny font (Blit16)
    static const FocusMetrics Standard;  ///< 320x240 TFT, 5x8 font
    static const FocusMetrics Large;     ///< 480x800 EPD, 24-32px font

    /// Set the active metrics for the application.
    static void set(const FocusMetrics& metrics);

    /// Get the active metrics. Returns Standard if not explicitly set.
    static const FocusMetrics& get();
};
