IF (ESP_PLATFORM)
    idf_component_register(
        SRC_DIRS "src/core" "src/settings" "src/text" "src/locale" "src/views" "src/controls" "src/viewcontrollers"
        PRIV_REQUIRES esp_timer
        INCLUDE_DIRS "include/core" "include/settings" "include/text" "include/locale" "include/views" "include/controls" "include/viewcontrollers"
    )
ELSE()
    file(GLOB_RECURSE FOCUS_SRCS src/*.cpp)
    add_library(focus STATIC ${FOCUS_SRCS})
    target_include_directories(focus PUBLIC
        include/core include/settings include/text include/locale
        include/views include/controls include/viewcontrollers)
ENDIF()
