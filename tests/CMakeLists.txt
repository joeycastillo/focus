cmake_minimum_required(VERSION 3.16)
project(focus-test CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FOCUS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# BDF fonts loaded at runtime by the glyph-provider tests. The bundled
# fixtures in tests/fonts/ (Lucida, Bigelow & Holmes — see
# fonts/LEGAL_NOTICE.txt) cover the suite; override to test against a
# different font set.
set(FOCUS_TEST_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fonts" CACHE PATH
    "Directory containing the BDF fonts the glyph-provider tests load")

if(NOT EXISTS "${FOCUS_TEST_FONTS_DIR}")
    message(WARNING
        "FOCUS_TEST_FONTS_DIR (${FOCUS_TEST_FONTS_DIR}) does not exist; "
        "the glyph-provider tests will fail at runtime.")
endif()

# --- All Focus framework sources (glob to avoid chasing transitive deps) ---
file(GLOB_RECURSE FOCUS_SOURCES CONFIGURE_DEPENDS ${FOCUS_ROOT}/src/*.cpp)

# --- Include directories ---
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FOCUS_ROOT}/include/core
    ${FOCUS_ROOT}/include/settings
    ${FOCUS_ROOT}/include/text
    ${FOCUS_ROOT}/include/locale
    ${FOCUS_ROOT}/include/views
    ${FOCUS_ROOT}/include/controls
    ${FOCUS_ROOT}/include/viewcontrollers
)

# --- Test source files ---
set(TEST_SOURCES
    main.cpp
    test_focus_config.cpp
    test_utf8.cpp
    test_measure_line_wrap.cpp
    test_color.cpp
    test_arabic_shaping.cpp
    test_unicode.cpp
    test_view_hierarchy.cpp
    test_invalidation.cpp
    test_event_dispatch.cpp
    test_stack_view_layout.cpp
    test_gesture_recognizer.cpp
    test_modal_reentrancy.cpp
    test_alert_identifiers.cpp
    test_tab_identifiers.cpp
    test_control_selected.cpp
    test_navigation_focus.cpp
    test_styled_glyph_provider.cpp
    test_fallback_glyph_provider.cpp
    test_regressions.cpp
    test_geometry.cpp
    test_clipping.cpp
    test_navigation_bar.cpp
    test_scroll_view.cpp
    test_text_view.cpp
    test_truncation.cpp
    test_shaping_invariant.cpp
    test_locale.cpp
    test_packed_font_memory.cpp
    test_task_busyness.cpp
)

# --- Build the test executable ---
add_executable(focus-test
    ${TEST_SOURCES}
    ${FOCUS_SOURCES}
)

target_include_directories(focus-test PRIVATE ${INCLUDE_DIRS})

# Trailing slash added here: tests concatenate filenames directly onto
# FONTS_DIR, and CMake strips trailing slashes from cached PATH values.
target_compile_definitions(focus-test PRIVATE
    FONTS_DIR="${FOCUS_TEST_FONTS_DIR}/"
)

target_compile_options(focus-test PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# --- BMP-only decoder configuration (own executable: UNICODE_BMP_ONLY changes
# the codepoint type at compile time, so it can't share focus-test's objects) ---
add_executable(utf8-bmp-test
    test_utf8_bmp.cpp
    ${FOCUS_ROOT}/src/text/Utf8.cpp
)
target_include_directories(utf8-bmp-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FOCUS_ROOT}/include/text
)
target_compile_definitions(utf8-bmp-test PRIVATE UNICODE_BMP_ONLY)
target_compile_options(utf8-bmp-test PRIVATE -Wall -Wextra)

# --- Filesystem-free build: whole framework compiled with FOCUS_HAS_FILESYSTEM=0,
#     plus a small main that exercises the memory factories. Proves the file code
#     is genuinely excludable and the fromMemory paths stand alone. ---
add_executable(focus-test-nofs
    test_no_filesystem.cpp
    ${FOCUS_SOURCES}
)
target_include_directories(focus-test-nofs PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(focus-test-nofs PRIVATE
    FOCUS_HAS_FILESYSTEM=0
    FONTS_DIR="${FOCUS_TEST_FONTS_DIR}/"
)
target_compile_options(focus-test-nofs PRIVATE -Wall -Wextra -Wno-unused-parameter)

# --- Shaping-free build: whole framework compiled with
#     FOCUS_HAS_TEXT_SHAPING=0. Proves the shaper and its tables are
#     genuinely excludable and measurement stays consistent (unshaped)
#     with rendering. ---
add_executable(focus-test-noshaping
    test_no_shaping.cpp
    ${FOCUS_SOURCES}
)
target_include_directories(focus-test-noshaping PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(focus-test-noshaping PRIVATE
    FOCUS_HAS_TEXT_SHAPING=0
    FONTS_DIR="${FOCUS_TEST_FONTS_DIR}/"
)
target_compile_options(focus-test-noshaping PRIVATE -Wall -Wextra -Wno-unused-parameter)

# --- Header include-order guard (compile-only) -----------------------------
# Every public header must compile when Arduino.h's round()/abs()/min()/max()
# function-like macros are already defined — i.e. when a translation unit
# reaches a Focus header AFTER <Arduino.h> (the natural order in an Arduino
# sketch). include/core/detail/stl_guard.h enforces this; this target is the
# regression check for it.
#
# Each public header is compiled in its own translation unit with the macros
# predefined and that header included FIRST, reproducing the worst-case order
# in which the header is the sole entry point into the standard library. A
# compile failure here means a header reached the STL while the macros were
# live: add `#include "detail/stl_guard.h"` as its first include (see any of
# the already-guarded roots, e.g. NotificationCenter.hpp).
#
# This is compile-only — an OBJECT library, built by `all`, with nothing to
# run. If it builds, every header is include-order-safe.
file(GLOB_RECURSE FOCUS_PUBLIC_HEADERS CONFIGURE_DEPENDS
    RELATIVE ${FOCUS_ROOT}/include ${FOCUS_ROOT}/include/*.hpp)

set(HEADER_GUARD_DIR ${CMAKE_CURRENT_BINARY_DIR}/header_guard)
set(HEADER_GUARD_SOURCES "")
foreach(hdr ${FOCUS_PUBLIC_HEADERS})
    # Headers are included by basename because the include dirs are flat -I
    # entries (one per subdir). Basenames are unique across the tree.
    get_filename_component(hdr_base ${hdr} NAME)
    string(MAKE_C_IDENTIFIER ${hdr} hdr_slug)
    set(gen_src ${HEADER_GUARD_DIR}/guard_${hdr_slug}.cpp)
    file(WRITE ${gen_src}
"// Generated by tests/CMakeLists.txt — do not edit.\n"
"// Arduino.h's function-like macros, defined before the Focus header below.\n"
"#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))\n"
"#define abs(x)   ((x)>0?(x):-(x))\n"
"#define min(a,b) ((a)<(b)?(a):(b))\n"
"#define max(a,b) ((a)>(b)?(a):(b))\n"
"#include \"${hdr_base}\"\n")
    list(APPEND HEADER_GUARD_SOURCES ${gen_src})
endforeach()

add_library(focus-header-guard OBJECT ${HEADER_GUARD_SOURCES})
target_include_directories(focus-header-guard PRIVATE ${INCLUDE_DIRS})
target_compile_options(focus-header-guard PRIVATE -Wall)
