cmake_minimum_required(VERSION 3.16)
project(focus-mp3player CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FOCUS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

file(GLOB_RECURSE FOCUS_SOURCES CONFIGURE_DEPENDS ${FOCUS_ROOT}/src/*.cpp)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/sdl/*.cpp)

find_package(SDL2 REQUIRED)

add_executable(mp3player ${APP_SOURCES} ${FOCUS_SOURCES})

target_include_directories(mp3player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FOCUS_ROOT}/include/core
    ${FOCUS_ROOT}/include/settings
    ${FOCUS_ROOT}/include/text
    ${FOCUS_ROOT}/include/locale
    ${FOCUS_ROOT}/include/views
    ${FOCUS_ROOT}/include/controls
    ${FOCUS_ROOT}/include/viewcontrollers
    ${SDL2_INCLUDE_DIRS})

if(TARGET SDL2::SDL2)
    target_link_libraries(mp3player PRIVATE SDL2::SDL2)
else()
    target_link_libraries(mp3player PRIVATE ${SDL2_LIBRARIES})
endif()

target_compile_options(mp3player PRIVATE -Wall -Wextra -Wno-unused-parameter)
