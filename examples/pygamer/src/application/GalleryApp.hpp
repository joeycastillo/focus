#pragma once

#include "Application.hpp"
#include <memory>

class DisplayST7735;

/// The example application: owns the display, registers the input and render
/// tasks, and installs the gallery inside a navigation controller.
class GalleryApp : public focus::Application {
public:
    GalleryApp(const std::shared_ptr<focus::Window>& window, std::shared_ptr<DisplayST7735> display);

    void setup() override;

private:
    std::shared_ptr<DisplayST7735> display;
};
