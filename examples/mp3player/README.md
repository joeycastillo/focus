# Focus MP3 Player

This is an MP3 player that runs on two SAMD51 boards from Adafruit, as well as on the desktop in an SDL window. You can drive it with a joystick on the Adafruit PyGamer, a fingertip on the Adafruit PyPortal, or a mouse and keyboard on your desktop.

The app layer is shared across all platforms, and in particular the PlayerApp and view controllers serve as a textbook example of how a Focus app is architected. Each platform supplies its own audio engine, which shows how you can achieve similar functionality across different platforms. The platforms also provide input tasks and display adapters suited for their unique input and display modalities. In the SAMD51 case, we even split these across two different boards to demonstrate how different hardware inputs and outputs are bridged over to Focus view hierarchies and interactions.

## Getting music onto it

Copy MP3 files (CBR recommended) to the music folder. On the boards: a `/music` folder on a FAT-formatted SD card. On the desktop: a `music` directory in the working directory you run it from, or pass `--music=<dir>`.

## PyGamer

    cd examples/mp3player
    pio run -e pygamer -t upload

| Input    | Action                       |
|----------|------------------------------|
| Joystick | Move focus                   |
| A        | Activate the focused control |
| B        | Navigate back                |

## PyPortal (classic, 320x240)

    cd examples/mp3player
    pio run -e pyportal -t upload

Tap a track to play it; tap the control buttons to control playback; tap the nav bar's back button to return to the library.

For good measure, the PyPortal's 8-bit parallel bus is fast enough that we can draw straight to the panel with that, illustrating that the framebuffer in the PyGamer version is totally optional as far as Focus is concerned.

## Desktop (SDL2)

    cd examples/mp3player
    cmake -S . -B build && cmake --build build -j
    ./build/mp3player [--input=touch|dpad] [--size=WxH] [--scale=N] [--music=<dir>]

`--input=touch` (default) maps the mouse to touch events; `--input=dpad` maps arrows/Enter/Escape to D-pad navigation, the same events the PyGamer sends. `--size=160x128` previews the PyGamer's geometry. `S` saves a PNG screenshot.

## What's in it

| Unit        | Files                         | Role                                                                                                                                   |
|-------------|-------------------------------|----------------------------------------------------------------------------------------------------------------------------------------|
| Shared app  | `src/app/`                    | `PlayerApp` (playlist + auto-advance), `PlayerEngine` interface, library and now-playing screens, one `RefreshTask` for every platform |
| Desktop     | `src/platform/sdl/`           | SDL display/input/audio (dr_mp3), runtime input-mode flag                                                                              |
| SAMD shared | `src/platform/samd/`          | Adafruit_MP3 library for MP3 decoding, SD file source, DAC output — shared by both boards                                              |
| PyGamer     | `src/platform/samd/pygamer/`  | ST7735 framebuffer adapter, joystick/button input                                                                                      |
| PyPortal    | `src/platform/samd/pyportal/` | direct-to-panel ILI9341 adapter, resistive touch input                                                                                 |

Footprint: PyGamer flash ~243 KB (47%) / RAM ~44 KB static (23%); PyPortal flash ~243 KB (24%) / RAM ~44 KB static (17%, no framebuffer).
