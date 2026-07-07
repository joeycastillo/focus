// examples/mp3player/src/platform/sdl/ScriptCommandProcessor.hpp
#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <cstdint>

namespace focus {
class Application;
class View;
class ViewController;
}

/// Platform operations the processor delegates (screenshot, quit).
class ScriptCommandDelegate {
public:
    virtual ~ScriptCommandDelegate() = default;
    virtual std::string handleScreenshot(const std::string& path) = 0;
    virtual std::string handleQuit() = 0;
};

/// Stdin command processor for scripted UI testing of the emulator.
///
/// Commands: tap <x y | identifier>, key <up|down|left|right|select|back|next|prev>,
/// focused, assert_screen <id>, wait <ms>, wait_for screen <id> <timeout_ms>,
/// screenshot [path], quit.
/// Responses go to stderr: "OK ..." / "ERR ...". A "" response means a
/// pending operation started (wait, wait_for, deferred tap TOUCH_UP).
/// Lines starting with '#' are comments.
class ScriptCommandProcessor {
public:
    ScriptCommandProcessor(focus::Application* application);

    void setDelegate(ScriptCommandDelegate* delegate);

    /// Process one command line; "" means a pending operation started.
    std::string processCommand(const std::string& line);

    /// True while a wait/wait_for/deferred TOUCH_UP is in flight.
    bool hasPendingOperation() const;

    /// Poll the pending operation; "" while still pending.
    std::string checkPendingOperation();

private:
    focus::Application* application;
    ScriptCommandDelegate* delegate = nullptr;

    enum class PendingType { None, Wait, WaitForScreen, TouchUp };
    PendingType pendingType = PendingType::None;
    std::chrono::steady_clock::time_point pendingDeadline;
    std::string pendingTarget;
    int32_t pendingTouchPacked = 0;
    std::string pendingResponse;

    std::shared_ptr<focus::View> resolveElement(const std::string& identifier);
    std::string activeScreenIdentifier();
    int32_t logicalToNativePacked(int lx, int ly);
    void generateTapAtPoint(int x, int y);

    std::string handleTap(const std::string& args);
    std::string handleKey(const std::string& args);
    std::string handleFocused();
    std::string handleWait(const std::string& args);
    std::string handleWaitFor(const std::string& args);
    std::string handleAssertScreen(const std::string& args);
};
