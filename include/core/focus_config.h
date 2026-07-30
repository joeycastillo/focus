/*
 * Compile-time capability flags for the Focus framework.
 * Safe to include from public headers.
 */
#pragma once

// FOCUS_HAS_FILESYSTEM: 1 where file loading (fstream/cstdio) is available and
// wanted, 0 on bare-metal targets where only memory loading is compiled in.
// An explicit -DFOCUS_HAS_FILESYSTEM=0/1 from the build always wins.
#if defined(FOCUS_HAS_FILESYSTEM)
    // honor the caller's value
#elif defined(ESP_PLATFORM)
    #define FOCUS_HAS_FILESYSTEM 1   // ESP-IDF and Arduino-ESP32
#elif defined(ARDUINO) || defined(FOCUS_PLATFORM_PICO)
    #define FOCUS_HAS_FILESYSTEM 0   // bare-metal SAMD / Pico
#else
    #define FOCUS_HAS_FILESYSTEM 1   // hosted: unit tests, desktop
#endif
