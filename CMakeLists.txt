idf_component_register(
    SRC_DIRS "src/core" "src/settings" "src/text" "src/locale" "src/views" "src/controls" "src/viewcontrollers"
    REQUIRES utf8_decode
    PRIV_REQUIRES esp_timer
    INCLUDE_DIRS "include/core" "include/settings" "include/text" "include/locale" "include/views" "include/controls" "include/viewcontrollers"
)
