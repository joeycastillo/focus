idf_component_register(
    SRC_DIRS "src/core" "src/settings" "src/text" "src/views" "src/controls" "src/viewcontrollers"
    REQUIRES utf8_decode tinyxml2
    INCLUDE_DIRS "include/core" "include/settings" "include/text" "include/views" "include/controls" "include/viewcontrollers"
)
