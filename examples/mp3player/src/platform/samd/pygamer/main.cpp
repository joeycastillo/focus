#include <Arduino.h>

#include "Window.hpp"
#include "app/PlayerApp.hpp"
#include "platform/samd/pygamer/DisplayST7735.hpp"
#include "platform/samd/pygamer/InputTask.hpp"
#include "platform/samd/SamdPlayerEngine.hpp"
#include "platform/samd/SamdPlayerEngineTask.hpp"

using namespace focus;

// PyGamer wiring
static constexpr uint8_t kSdChipSelect = 4;
static constexpr uint8_t kSpeakerEnable = 51;

static std::shared_ptr<PlayerApp> app;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 2000) {}

    auto display = std::make_shared<DisplayST7735>();
    auto window = std::make_shared<Window>(display,
        MakeSize(display->getWidth(), display->getHeight()));
    auto engine = std::make_shared<SamdPlayerEngine>(kSdChipSelect, kSpeakerEnable);

    app = std::make_shared<PlayerApp>(window, display, engine);
    app->addTask(std::make_shared<InputTask>());
    app->addTask(std::make_shared<SamdPlayerEngineTask>(engine));

    app->run();  // cooperative run loop; never returns
}

void loop() {}
