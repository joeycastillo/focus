// examples/mp3player/src/platform/sdl/ScriptCommandProcessor.cpp
#include "platform/sdl/ScriptCommandProcessor.hpp"
#include "Application.hpp"
#include "View.hpp"
#include "ViewController.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Focus.hpp"
#include <cstdio>

using namespace focus;

ScriptCommandProcessor::ScriptCommandProcessor(Application* application)
    : application(application) {
}

void ScriptCommandProcessor::setDelegate(ScriptCommandDelegate* delegate) {
    this->delegate = delegate;
}

std::string ScriptCommandProcessor::processCommand(const std::string& line) {
    if (!line.empty() && line[0] == '#') return "";  // comment line

    std::string cmd = line;
    std::string args;
    size_t space = line.find(' ');
    if (space != std::string::npos) {
        cmd = line.substr(0, space);
        args = line.substr(space + 1);
    }

    if (cmd == "tap") return this->handleTap(args);
    if (cmd == "key") return this->handleKey(args);
    if (cmd == "focused") return this->handleFocused();
    if (cmd == "wait") return this->handleWait(args);
    if (cmd == "wait_for") return this->handleWaitFor(args);
    if (cmd == "assert_screen") return this->handleAssertScreen(args);
    if (cmd == "screenshot" && this->delegate) {
        return this->delegate->handleScreenshot(args);
    }
    if (cmd == "quit" && this->delegate) {
        return this->delegate->handleQuit();
    }
    return "ERR unknown command: " + cmd;
}

bool ScriptCommandProcessor::hasPendingOperation() const {
    return this->pendingType != PendingType::None;
}

std::string ScriptCommandProcessor::checkPendingOperation() {
    if (this->pendingType == PendingType::None) return "";

    auto now = std::chrono::steady_clock::now();

    if (this->pendingType == PendingType::Wait) {
        if (now >= this->pendingDeadline) {
            this->pendingType = PendingType::None;
            return "OK wait";
        }
        return "";
    }

    if (this->pendingType == PendingType::WaitForScreen) {
        std::string current = this->activeScreenIdentifier();
        if (current == this->pendingTarget) {
            this->pendingType = PendingType::None;
            return "OK wait_for screen " + this->pendingTarget;
        }
        if (now >= this->pendingDeadline) {
            this->pendingType = PendingType::None;
            return "ERR wait_for timeout (wanted " + this->pendingTarget +
                   ", got " + current + ")";
        }
        return "";
    }

    if (this->pendingType == PendingType::TouchUp) {
        // TOUCH_UP fires one frame after TOUCH_DOWN so gesture buffering settles.
        this->application->generateEvent(FOCUS_EVENT_TOUCH_UP, this->pendingTouchPacked);
        this->pendingType = PendingType::None;
        return this->pendingResponse;
    }

    return "";
}

std::shared_ptr<View> ScriptCommandProcessor::resolveElement(const std::string& identifier) {
    auto window = this->application->getWindow();
    if (!window) return nullptr;
    return findAccessibilityElement(window, identifier);
}

std::string ScriptCommandProcessor::activeScreenIdentifier() {
    auto vc = this->application->activeViewController();
    return vc ? vc->accessibilityIdentifier : "";
}

int32_t ScriptCommandProcessor::logicalToNativePacked(int lx, int ly) {
    // generateEvent applies the native->logical rotation; apply its inverse
    // so scripted logical coordinates arrive unchanged.
    int nx = lx, ny = ly;
    if (auto display = this->application->getWindow()->getDisplay().lock()) {
        int nw = display->getNativeWidth();
        int nh = display->getNativeHeight();
        switch (display->getRotation()) {
            case 0:  nx = lx;           ny = ly;           break;
            case 1:  nx = nw - 1 - ly;  ny = lx;           break;
            case 2:  nx = nw - 1 - lx;  ny = nh - 1 - ly;  break;
            case 3:  nx = ly;           ny = nh - 1 - lx;  break;
        }
    }
    return (nx << 16) | ny;
}

void ScriptCommandProcessor::generateTapAtPoint(int x, int y) {
    int32_t packed = this->logicalToNativePacked(x, y);
    this->application->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packed);
    this->pendingType = PendingType::TouchUp;
    this->pendingTouchPacked = packed;
}

std::string ScriptCommandProcessor::handleTap(const std::string& args) {
    int x, y;
    if (sscanf(args.c_str(), "%d %d", &x, &y) == 2) {
        this->pendingResponse = "OK tap " + std::to_string(x) + " " + std::to_string(y);
        this->generateTapAtPoint(x, y);
        return "";
    }

    auto element = this->resolveElement(args);
    if (!element) return "ERR tap: element not found: " + args;

    Rect rect = element->accessibilityRect();
    x = rect.origin.x + rect.size.width / 2;
    y = rect.origin.y + rect.size.height / 2;
    this->pendingResponse = "OK tap " + args +
        " (" + std::to_string(x) + "," + std::to_string(y) + ")";
    this->generateTapAtPoint(x, y);
    return "";
}

std::string ScriptCommandProcessor::handleKey(const std::string& args) {
    if (args == "up")     { this->application->generateEvent(FOCUS_EVENT_DIRECTION_UP, 0);    return "OK key up"; }
    if (args == "down")   { this->application->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  return "OK key down"; }
    if (args == "left")   { this->application->generateEvent(FOCUS_EVENT_DIRECTION_LEFT, 0);  return "OK key left"; }
    if (args == "right")  { this->application->generateEvent(FOCUS_EVENT_DIRECTION_RIGHT, 0); return "OK key right"; }
    if (args == "select") { this->application->generateEvent(FOCUS_EVENT_SELECT, 0);          return "OK key select"; }
    if (args == "back")   { this->application->generateEvent(FOCUS_EVENT_BACK, 0);            return "OK key back"; }
    if (args == "next") { this->application->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);     return "OK key next"; }
    if (args == "prev") { this->application->generateEvent(FOCUS_EVENT_ACCESSIBILITY_PREVIOUS, 0); return "OK key prev"; }
    return "ERR unknown key: " + args;
}

std::string ScriptCommandProcessor::handleFocused() {
    auto window = this->application->getWindow();
    auto focused = window->getFocusedView().lock();
    if (!focused || focused.get() == window.get()) return "OK focused (window)";
    if (!focused->accessibilityIdentifier.empty()) {
        return "OK focused " + focused->accessibilityIdentifier;
    }
    std::string label = focused->accessibilityLabel();
    if (!label.empty()) return "OK focused " + label;
    return "OK focused (unnamed)";
}

std::string ScriptCommandProcessor::handleWait(const std::string& args) {
    int ms;
    if (sscanf(args.c_str(), "%d", &ms) != 1 || ms < 0) {
        return "ERR wait: expected <ms>";
    }
    this->pendingType = PendingType::Wait;
    this->pendingDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    return "";
}

std::string ScriptCommandProcessor::handleWaitFor(const std::string& args) {
    char identifier[256];
    int timeoutMs;
    if (sscanf(args.c_str(), "screen %255s %d", identifier, &timeoutMs) == 2) {
        std::string current = this->activeScreenIdentifier();
        if (current == std::string(identifier)) {
            return "OK wait_for screen " + std::string(identifier);
        }
        this->pendingType = PendingType::WaitForScreen;
        this->pendingTarget = identifier;
        this->pendingDeadline = std::chrono::steady_clock::now() +
                                std::chrono::milliseconds(timeoutMs);
        return "";
    }
    return "ERR wait_for: expected 'screen <identifier> <timeout_ms>'";
}

std::string ScriptCommandProcessor::handleAssertScreen(const std::string& args) {
    std::string current = this->activeScreenIdentifier();
    if (current == args) return "OK assert_screen " + args;
    return "ERR assert_screen: expected " + args + ", got " + current;
}
