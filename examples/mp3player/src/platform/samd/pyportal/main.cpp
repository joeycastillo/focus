#include <Arduino.h>

#include "Window.hpp"
#include "app/PlayerApp.hpp"
#include "platform/samd/pyportal/DisplayILI9341.hpp"
#include "platform/samd/pyportal/TouchInputTask.hpp"
#include "platform/samd/SamdPlayerEngine.hpp"
#include "platform/samd/SamdPlayerEngineTask.hpp"

using namespace focus;

// PyPortal wiring
static constexpr uint8_t kSdChipSelect = 32;
static constexpr uint8_t kSpeakerEnable = 50;

static std::shared_ptr<PlayerApp> app;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 2000) {}

    auto display = std::make_shared<DisplayILI9341>();
    auto window = std::make_shared<Window>(display,
        MakeSize(display->getWidth(), display->getHeight()));
    window->setTouchEnabled();

    auto engine = std::make_shared<SamdPlayerEngine>(kSdChipSelect, kSpeakerEnable);

    app = std::make_shared<PlayerApp>(window, display, engine);
    app->addTask(std::make_shared<TouchInputTask>());
    app->addTask(std::make_shared<SamdPlayerEngineTask>(engine));

    app->run();  // cooperative run loop; never returns
}

void loop() {}
