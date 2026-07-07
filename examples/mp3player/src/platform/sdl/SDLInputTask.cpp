#include "platform/sdl/SDLInputTask.hpp"
#include "platform/sdl/SDLDisplay.hpp"
#include "Application.hpp"
#include "Focus.hpp"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/select.h>
#include <unistd.h>

using namespace focus;

// Window pixels -> display pixels -> packed (x << 16) | y.
static int32_t packMouse(int wx, int wy, int scale) {
    int x = wx / scale;
    int y = wy / scale;
    return (x << 16) | y;
}

SDLInputTask::SDLInputTask(std::shared_ptr<SDLDisplay> display, InputMode mode,
                           int scale, bool scriptMode)
    : display(display), mode(mode), scale(scale), scriptMode(scriptMode) {}

bool SDLInputTask::run(std::shared_ptr<Application> application) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                exit(0);
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    display->flush({{0, 0}, {0, 0}});
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (mode != InputMode::DPad && event.button.button == SDL_BUTTON_LEFT) {
                    application->generateEvent(FOCUS_EVENT_TOUCH_DOWN,
                        packMouse(event.button.x, event.button.y, scale));
                }
                break;
            case SDL_MOUSEMOTION:
                if (mode != InputMode::DPad && (event.motion.state & SDL_BUTTON_LMASK)) {
                    application->generateEvent(FOCUS_EVENT_TOUCH_MOVED,
                        packMouse(event.motion.x, event.motion.y, scale));
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (mode != InputMode::DPad && event.button.button == SDL_BUTTON_LEFT) {
                    application->generateEvent(FOCUS_EVENT_TOUCH_UP,
                        packMouse(event.button.x, event.button.y, scale));
                }
                break;
            case SDL_MOUSEWHEEL:
                // The wheel is a rotary encoder: physical wheel-down = next element.
                if (mode == InputMode::Hybrid) {
                    int wheelY = event.wheel.y;
                    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) wheelY = -wheelY;
                    if (wheelY < 0) {
                        application->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
                    } else if (wheelY > 0) {
                        application->generateEvent(FOCUS_EVENT_ACCESSIBILITY_PREVIOUS, 0);
                    }
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_DIRECTION_UP, 0);
                        break;
                    case SDLK_DOWN:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
                        break;
                    case SDLK_LEFT:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_DIRECTION_LEFT, 0);
                        break;
                    case SDLK_RIGHT:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_DIRECTION_RIGHT, 0);
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_SELECT, 0);
                        break;
                    case SDLK_ESCAPE:
                        if (mode != InputMode::Touch)
                            application->generateEvent(FOCUS_EVENT_BACK, 0);
                        break;
                    case SDLK_TAB:
                        if (mode == InputMode::Hybrid) {
                            application->generateEvent(
                                (event.key.keysym.mod & KMOD_SHIFT)
                                    ? FOCUS_EVENT_ACCESSIBILITY_PREVIOUS
                                    : FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
                        }
                        break;
                    case SDLK_s: {
                        char filename[64];
                        snprintf(filename, sizeof(filename),
                                 "screenshot_%03d.png", screenshotCounter++);
                        if (display->savePNG(filename)) {
                            fprintf(stderr, "Screenshot saved: %s\n", filename);
                        }
                        break;
                    }
                }
                break;
        }
    }
    if (scriptMode) {
        if (!this->processor) {
            this->processor = std::make_unique<ScriptCommandProcessor>(application.get());
            this->processor->setDelegate(this);
        }
        if (this->processor->hasPendingOperation()) {
            std::string result = this->processor->checkPendingOperation();
            if (!result.empty()) fprintf(stderr, "%s\n", result.c_str());
        } else if (this->hasStdinData()) {
            this->processNextCommand();
        }
    }
    return false;
}

bool SDLInputTask::hasStdinData() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {0, 0};
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void SDLInputTask::processNextCommand() {
    char line[1024];
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "EOF on stdin, exiting\n");
        exit(0);
    }
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
    if (strlen(line) == 0) return;

    std::string result = this->processor->processCommand(std::string(line));
    if (!result.empty()) fprintf(stderr, "%s\n", result.c_str());
}

std::string SDLInputTask::handleScreenshot(const std::string& path) {
    if (path.empty()) {
        char filename[64];
        snprintf(filename, sizeof(filename), "screenshot_%03d.png", screenshotCounter++);
        if (display->savePNG(filename)) return std::string("OK screenshot ") + filename;
        return "ERR screenshot failed";
    }
    if (display->savePNG(path.c_str())) return "OK screenshot " + path;
    return "ERR screenshot failed";
}

std::string SDLInputTask::handleQuit() {
    fprintf(stderr, "OK quit\n");
    exit(0);
}
