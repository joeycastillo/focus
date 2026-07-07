#include <cstdio>
#include <cstring>
#include <string>
#include <memory>
#include <SDL.h>

#include "Focus.hpp"
#include "Window.hpp"
#include "app/PlayerApp.hpp"
#include "platform/sdl/SDLDisplay.hpp"
#include "platform/sdl/SDLInputTask.hpp"
#include "platform/sdl/SDLYieldTask.hpp"
#include "platform/sdl/SdlPlayerEngine.hpp"
#include "platform/sdl/SdlPlayerEngineTask.hpp"

using namespace focus;

int main(int argc, char* argv[]) {
    int width = 320, height = 240, scale = 2;
    InputMode mode = InputMode::Touch;
    bool scriptMode = false;
    std::string musicDir = "./music";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            fprintf(stderr, "Usage: mp3player [options]\n");
            fprintf(stderr, "  --input=touch|dpad|hybrid  Input mode (default: touch)\n");
            fprintf(stderr, "  --size=WxH          Display size (default: 320x240)\n");
            fprintf(stderr, "  --scale=N           Integer window zoom (default: 2)\n");
            fprintf(stderr, "  --music=<dir>       MP3 directory (default: ./music)\n");
            fprintf(stderr, "  --script            Read script commands from stdin\n");
            fprintf(stderr, "Keys: arrows/enter/esc (dpad mode), mouse (touch mode), S = screenshot\n");
            fprintf(stderr, "Hybrid adds: Tab/Shift-Tab and scroll wheel = next/previous element\n");
            return 0;
        } else if (arg == "--input=touch") {
            mode = InputMode::Touch;
        } else if (arg == "--input=dpad") {
            mode = InputMode::DPad;
        } else if (arg == "--input=hybrid") {
            mode = InputMode::Hybrid;
        } else if (arg == "--script") {
            scriptMode = true;
        } else if (arg.rfind("--size=", 0) == 0) {
            if (sscanf(arg.c_str(), "--size=%dx%d", &width, &height) != 2) {
                fprintf(stderr, "Bad --size, expected WxH\n");
                return 1;
            }
        } else if (arg.rfind("--scale=", 0) == 0) {
            scale = atoi(arg.c_str() + 8);
            if (scale < 1) scale = 1;
        } else if (arg.rfind("--music=", 0) == 0) {
            musicDir = arg.substr(8);
        } else {
            fprintf(stderr, "Unknown option: %s\n", arg.c_str());
            return 1;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* sdlWindow = SDL_CreateWindow("Focus MP3 Player",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width * scale, height * scale, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!sdlWindow || !renderer) {
        fprintf(stderr, "SDL window/renderer failed: %s\n", SDL_GetError());
        return 1;
    }

    auto display = std::make_shared<SDLDisplay>(width, height, renderer);
    auto window = std::make_shared<Window>(display, MakeSize(width, height));
    if (mode != InputMode::DPad) {
        window->setTouchEnabled();
    }

    auto engine = std::make_shared<SdlPlayerEngine>(musicDir);
    auto app = std::make_shared<PlayerApp>(window, display, engine);
    app->addTask(std::make_shared<SDLInputTask>(display, mode, scale, scriptMode));
    app->addTask(std::make_shared<SdlPlayerEngineTask>(engine));
    app->addTask(std::make_shared<SDLYieldTask>());
    app->run();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return 0;
}
