#include "app/RefreshTask.hpp"

#include "Application.hpp"
#include "Window.hpp"

using namespace focus;

RefreshTask::RefreshTask(std::shared_ptr<Display> display) : display(display) {}

bool RefreshTask::run(std::shared_ptr<Application> application) {
    Window* window = application->getWindow().get();
    Rect dirtyRect = window->getDirtyRect();

    if (dirtyRect.size.width && dirtyRect.size.height) {
        window->draw(0, 0, dirtyRect);      // view tree -> display
        display->flush(dirtyRect);          // dirty region -> panel
        window->clearNeedsDisplay();
    }

    return false;  // long-lived task, never removed
}
