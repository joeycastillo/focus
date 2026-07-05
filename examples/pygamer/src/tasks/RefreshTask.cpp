#include "RefreshTask.hpp"

#include <Arduino.h>
#include <cstdio>

#include "Application.hpp"
#include "Window.hpp"
#include "display/DisplayST7735.hpp"

using namespace focus;

RefreshTask::RefreshTask(std::shared_ptr<DisplayST7735> display) : display(display) {}

bool RefreshTask::run(std::shared_ptr<Application> application) {
    Window* window = application->getWindow().get();
    Rect dirtyRect = window->getDirtyRect();

    if (dirtyRect.size.width && dirtyRect.size.height) {
        window->draw(0, 0, dirtyRect);      // view tree -> framebuffer
        display->flush(dirtyRect);          // dirty region -> panel
        window->clearNeedsDisplay();
    }

    return false;  // long-lived task, never removed
}
