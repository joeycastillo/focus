#include <Arduino.h>

#include "Window.hpp"
#include "application/GalleryApp.hpp"
#include "display/DisplayST7735.hpp"

using namespace focus;

static std::shared_ptr<GalleryApp> app;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 2000) {}

    auto display = std::make_shared<DisplayST7735>();
    auto window = std::make_shared<Window>(display, MakeSize(display->getWidth(), display->getHeight()));
    app = std::make_shared<GalleryApp>(window, display);

    app->run();  // cooperative run loop; never returns
}

void loop() {}
