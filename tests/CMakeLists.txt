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
    test_utf8.cpp
    test_measure_line_wrap.cpp
    test_color.cpp
    test_arabic_shaping.cpp
    test_unicode.cpp
    test_view_hierarchy.cpp
    test_event_dispatch.cpp
    test_stack_view_layout.cpp
    test_gesture_recognizer.cpp
    test_modal_reentrancy.cpp
    test_control_selected.cpp
    test_styled_glyph_provider.cpp
    test_fallback_glyph_provider.cpp
    test_regressions.cpp
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
