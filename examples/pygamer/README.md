# Focus on the Adafruit PyGamer

A PlatformIO example on the PyGamer's ATSAMD51J19 (Cortex-M4F, 192 KB RAM, 512 KB flash) with a 160x128 ST7735 TFT.

## Controls

| Input | Action |
|---|---|
| Joystick | Move focus between controls |
| A | Activate the focused control |
| B | Navigate back |

## Build and upload

Requires [PlatformIO Core](https://platformio.org/):

    cd examples/pygamer
    pio run -t upload
    pio device monitor -b 115200   # optional: Focus logs over USB serial

## What's in it

This example is designed to show the basics of how to set Focus up on your own hardware: creating your own Display subclass along with tasks for input and display refresh, and setting up a view controller with controls that the user can navigate through. The source is organized by role, the way a larger Focus app would be:

| Unit | File | Role |
|---|---|---|
| Entry point | `src/main.cpp` | brings up Serial + the display, then hands off to `GalleryApp` |
| Application | `src/application/GalleryApp.cpp` | registers the input + render tasks and installs the root view controller |
| View controllers | `src/viewcontrollers/GalleryViewController.cpp`, `AboutViewController.cpp` | the Button/Checkbox/Slider gallery and its push/pop About screen |
| Display adapter | `src/display/DisplayST7735.cpp` | RGB565 framebuffer (40 KB); `flush()` pushes dirty regions over SPI |
| Refresh task | `src/tasks/RefreshTask.cpp` | (if needed) draw dirty rect -> flush -> clear, once per runloop |
| Input task | `src/tasks/InputTask.cpp` | read analog joystick + buttons -> turn into Focus events |

Footprint on SAMD51 (`pio run` summary): Flash ~177 KB (34.6% of 512 KB), RAM ~4 KB static (2.2%), plus a 40 KB framebuffer allocated at startup.
