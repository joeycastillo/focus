# Focus

Focus is a lightweight, platform-agnostic UI framework for resource-constrained embedded systems, inspired by UIKit circa 2008 and AppKit before that. It provides a view hierarchy with layout, controls, text rendering, event dispatch, and cooperative task scheduling — everything needed to build interactive applications on ESP32-like microcontroller-oriented devices (read: hundreds of kilobytes to single-digit megabytes of RAM).

If you ever wrote an iOS app back in the day, Focus should feel instantly familiar: a view hierarchy with view controllers and lifecycle callbacks, target-action controls with state-keyed content, navigation stacks, modals and localizable strings.

Focus is single-threaded and cooperative: there are no background threads or preemptive scheduling. The application run loop calls registered tasks one at a time, and each task yields quickly so the loop stays responsive. This makes Focus predictable and easy to reason about on bare-metal or RTOS targets.

**Key features:**

- View hierarchy with dirty-rect tracking for efficient partial redraws
- Touch input (hit testing, capture, swipe detection) and directional navigation (d-pad / arrow keys)
- Built-in controls: Button, TextField, Checkbox, RadioButton, Slider
- Automatic layout with VStack / HStack
- Unicode text rendering with Arabic shaping and bidirectional support
- Multiple font formats: BDF, packed BDF (BDP), and Unifont
- View controller lifecycle (create on demand, destroy on dismiss)
- Navigation stack, tabs, and modal alerts
- Pluggable settings persistence and string localization
- Publish-subscribe notifications with automatic owner-based cleanup
- Accessibility tree with semantic roles and identifiers

Focus requires C++17 and uses `std::shared_ptr` for view ownership.

## Platforms

Focus was developed as an ESP-IDF component, but the repository also includes a `library.json` so that PlatformIO projects can consume it directly. An example in [examples/pygamer](examples/pygamer/), shows the framework runnnuing on the Adafruit PyGamer (ATSAMD51 + ST7735 TFT, 192 KB RAM, 512 KB flash). [examples/mp3player](examples/mp3player/) is an MP3 player that runs on PyGamer (D-pad), PyPortal (touch), and your desktop (SDL emulator) from one shared app.

## Core Concepts

### View Hierarchy

Every visual element in Focus is a **View**. Views form a tree: each view has zero or more subviews and at most one superview. Subviews are owned via `std::shared_ptr<View>`, so adding a subview keeps it alive.

Every view has two rectangles:
- **frame**: position and size in the superview's coordinate system.
- **bounds**: the view's own coordinate system (origin is normally 0,0 but can be offset for scrolling).

Views have a foreground color and background color (`uint16_t` values — see Colors and Display below). If a view is **opaque**, it fills its frame with its background color before drawing content.

### Window

A **Window** is a special View that sits at the root of the hierarchy. It bridges the view tree to a **Display** backend. The Window tracks a **dirty rectangle**: the union of all regions that need redrawing. When the display is ready, the application draws only the dirty region, not the entire screen.

In non-touch-oriented applications, Window also manages focus: which view currently receives keyboard/d-pad events.

### Application

**Application** is the central coordinator. It owns the Window, manages a root view controller plus a stack of modally presented view controllers, and runs the cooperative event loop.

```
while running and tasks.count > 0:
    for each task in tasks:
        if task.run() returns true:
            remove task (one-shot)
    loopCounter++
```

Subclass Application and override `setup()` to set your root view controller and add tasks. Call `run()` to enter the loop. The loop exits when `quit()` is called, or when the last task removes itself.

### ViewController

A **ViewController** manages a view's lifecycle. Views are created lazily (on first appearance) and destroyed when the view controller disappears. This keeps memory usage low since only the visible screen's views are alive.

The lifecycle sequence:

1. `createView()`: build the view hierarchy (called once, lazily)
2. `viewWillAppear()`: about to become visible
3. `viewDidLayoutSubviews()`: frame has been finalized; do size-dependent work
4. `viewDidAppear()`: now on screen
5. `viewWillDisappear()`: about to be removed
6. `viewDidDisappear()`: removed; default implementation calls `destroyView()`

State that must survive across appearances should be stored in the **controller**, not the view.

### Cooperative Tasks

Tasks are registered with `Application::addTask()`. Each task's `run()` method is called once per loop iteration. Return `false` to keep running, `true` to self-remove. Tasks must yield quickly to keep the UI responsive.

Focus provides three task types:
- **Task**: base class; subclass for custom background work
- **Timer**: fires a callback at intervals or on calendar boundaries
- **DeferredTask**: runs a callback after N loop iterations, then removes itself

## Getting Started

Here is a minimal Focus application. You will need to provide a `Display` subclass for your platform (e-paper, LCD, SDL, etc.).

```cpp
#include "Application.hpp"
#include "ViewController.hpp"
#include "Window.hpp"
#include "Color.hpp"
#include "Button.hpp"
#include "VStack.hpp"
#include "LabelView.hpp"
#include "Font.hpp"

// --- Your platform's Display subclass ---
// (implements fillRect and blitMasked; optionally overrides blitOpaque)
#include "MyDisplay.hpp"

// --- Your platform's input task (touch or or D-pad input) ---
// A Task that calls app->generateEvent(int32_t eventType, int32_t userInfo)
// with events from the input hardware (touchscreen or D-pad). Focus turns
// these raw Events into interactions with the on-screen controls.
#include "MyTouchInput.hpp"

// --- Your platform's render task ---
// A Task that, once per loop iteration, draws the window's dirty region
// (window->draw) and presents it.
#include "MyRefreshTask.hpp"

// --- A simple view controller ---
class HelloViewController : public ViewController {
public:
    using ViewController::ViewController;

protected:
    void createView() override {
        auto stack = std::make_shared<VStack>(RectZero);
        stack->setSpacing(8);
        stack->setMargins(16);

        auto label = std::make_shared<LabelView>(
            MakeRect(0, 0, 0, 24), "Hello, Focus!");
        stack->addSubview(label);

        auto button = std::make_shared<Button>(
            MakeRect(0, 0, 0, 48), "Quit");
        std::weak_ptr<Application> weakApp = this->application;
        button->setAction([weakApp](Event, std::weak_ptr<View>) {
            if (auto app = weakApp.lock()) app->quit();
        }, FOCUS_EVENT_TOUCH_UP_INSIDE);
        stack->addSubview(button);

        this->view = stack;
    }
};

// --- The application ---
class HelloApp : public Application {
public:
    HelloApp(std::shared_ptr<Window> window, std::shared_ptr<MyDisplay> display) : Application(window), display(display) {}

    void setup() override {
        auto vc = std::make_shared<HelloViewController>(this->shared_from_this());
        this->setRootViewController(vc);

        // add all tasks in setup()
        this->addTask(std::make_shared<MyTouchInput>());
        this->addTask(std::make_shared<MyRefreshTask>(this->display));
    }

private:
    std::shared_ptr<MyDisplay> display;
};

// --- Entry point ---
int main() {
    auto display = std::make_shared<MyDisplay>(/* ... */);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    window->setTouchEnabled();  // omit for d-pad driven interaction

    auto app = std::make_shared<HelloApp>(window, display);
    app->run();

    return 0;
}
```

## Layout

### Manual Positioning

Set a view's position and size directly with `setFrame()`:

```cpp
auto label = std::make_shared<LabelView>(MakeRect(20, 10, 200, 32), "Title");
```

### VStack and HStack

**VStack** arranges children top-to-bottom. **HStack** arranges them left-to-right. Both support fixed and flexible sizing:

- **Fixed size**: set the dimension along the stack axis to a nonzero value. A child with height 48 in a VStack always gets exactly 48 pixels.
- **Flexible size**: set the dimension to 0. All flexible children share the remaining space equally after fixed children and spacing are accounted for.
- **Cross-axis**: children always fill the stack's full width (VStack) or height (HStack).

```cpp
auto stack = std::make_shared<VStack>(MakeRect(0, 0, 480, 800));
stack->setSpacing(8);       // 8px between items (not at edges)
stack->setMargins(16);      // 16px inset on all four sides

// Fixed height, always 48px tall
auto button = std::make_shared<Button>(MakeRect(0, 0, 0, 48), "OK");
stack->addSubview(button);

// Flexible height, fills remaining space
auto content = std::make_shared<View>(RectZero);
stack->addSubview(content);
```

For horizontal rows inside a vertical layout, nest an HStack:

```cpp
auto row = std::make_shared<HStack>(MakeRect(0, 0, 0, 48));
row->setSpacing(8);

auto label = std::make_shared<LabelView>(RectZero, "Name:");  // flexible width
row->addSubview(label);

auto field = std::make_shared<TextField>(MakeRect(0, 0, 200, 0));  // fixed 200px
row->addSubview(field);

stack->addSubview(row);
```

## Views

Focus includes these built-in views:

| View | Description |
|------|-------------|
| **LabelView** | Renders a text string with word wrapping and alignment. |
| **CanvasView** | Pixel buffer with drawing primitives (rect, circle, text, drawMask, invert). |
| **BitmapView** | Displays a 1bpp bitmap from an external data pointer. |
| **ProgressView** | Horizontal progress bar (0.0 to 1.0). |
| **BorderedView** | Container with a 1px border. |
| **HatchedView** | Semi-transparent overlay for dimming content behind modals. |
| **MaskView** | Renders foreground color where mask bits are set, background elsewhere. |
| **NavigationBar** | Title bar with optional back button and right action button. |
| **KeyboardView** | On-screen keyboard for text input. |
| **CollectionView** | Abstract base for data-source-driven collections; holds the data source, delegate, and layout configuration. |
| **PagedCollectionView** | Concrete list or grid that lays items out in fixed pages; navigate with `goToPage()`. |
| **PaginatedCollectionView** | A view that wraps a PagedCollectionView and adds pagination controls (arrows or a footer). |

### CollectionView

CollectionView is an abstract base — it can't be instantiated directly. For a paginated list or grid, instantiate **PagedCollectionView** (or **PaginatedCollectionView** when you want on-screen pagination controls). All three share the same data source / delegate pattern:

```cpp
class MyDataSource : public CollectionViewDataSource {
    size_t numberOfItems(CollectionView*) override { return items.size(); }

    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(
        CollectionView*, size_t index, Rect frame) override
    {
        auto cell = std::make_shared<CollectionViewCell>(frame);
        auto label = std::make_shared<LabelView>(frame, items[index].name);
        cell->addSubview(label);
        return cell;
    }
};
```

Configure layout with `setLayout()` (VerticalList, HorizontalList, or Grid), `setItemSize()`, and `setItemSpacing()`. Call `reloadData()` after data changes. PagedCollectionView adds page navigation (`goToPage()`, `getCurrentPage()`, `getPageCount()`); PaginatedCollectionView forwards the same configuration methods and adds a pagination style (`None`, `Arrows`, or `Footer`).

## Controls

All controls inherit from **Control**, which adds enabled/disabled and selected/unselected state and focus behavior to View. Controls fire **actions**: callbacks registered for specific event types.

### Action Callbacks

```cpp
button->setAction([](Event event, std::weak_ptr<View> sender) {
    // Handle tap
}, FOCUS_EVENT_TOUCH_UP_INSIDE);
```

The callback receives the Event (with type and userInfo) and a weak pointer to the sender view. Use `sender.lock()` if you need to inspect the control.

**Ownership tracking**: actions can optionally track an owner's lifetime. If the owner is destroyed, the action is automatically invalidated:

```cpp
button->setAction(callback, FOCUS_EVENT_TOUCH_UP_INSIDE,
                  this->shared_from_this());  // invalidates when `this` is destroyed
```

### Common Event Types

| Event | Used by |
|-------|---------|
| `FOCUS_EVENT_TOUCH_UP_INSIDE` | Button tap, cell selection |
| `FOCUS_EVENT_VALUE_CHANGED` | Slider, Checkbox, RadioGroup |
| `FOCUS_EVENT_SELECT` | D-pad confirm (falls back to TOUCH_UP_INSIDE) |

### Built-in Controls

| Control | Description |
|---------|-------------|
| **Button** | Tappable button with text label. Inverts colors when focused. Supports a `selected` visual state via `setSelected()` — when selected, renders inverted (filled, no border), same as focused. The caller manages toggle semantics in action handlers. Supports state-keyed content: `setTitle(text, state)` and `setImage(mask, size, state)` register different content for the Normal and Selected states, with fallback to Normal. |
| **CircularButton** | Circular icon button. Renders icon or text in a circle outline (normal) or filled circle with cutout content (highlighted). For toolbar and toggle button UIs. |
| **TextField** | Single-line text input. Presents an on-screen keyboard when focused. |
| **PasswordField** | TextField that displays bullets instead of characters. |
| **Checkbox** | Toggle with text label. Fires VALUE_CHANGED on toggle. Query state with `isSelected()` / `setSelected()`. |
| **RadioButton** | Radio button. Use with **RadioGroup** for mutual exclusion. |
| **Slider** | Horizontal slider (0.0 to 1.0). userInfo carries the float value bit-cast to int32_t. |

## View Controllers

### Lifecycle

Override `createView()` to build your view hierarchy. The other lifecycle methods are available for setup and teardown:

```cpp
class MyViewController : public ViewController {
protected:
    void createView() override {
        // Build view tree, assign to this->view
        this->view = std::make_shared<VStack>(RectZero);
    }

public:
    void viewWillAppear() override {
        ViewController::viewWillAppear();  // triggers createView() if needed
        // Load data, register notification observers
    }

    void viewDidLayoutSubviews() override {
        // Frame is now final; do size-dependent work (e.g. reload collection)
    }

    void viewDidAppear() override {
        // We are on screen! Start timers, etc.
    }

    void viewWillDisappear() override {
        // Save state before removal
    }
};
```

### NavigationViewController

Push/pop stack for drill-down navigation:

```cpp
auto nav = NavigationViewController::create(app, rootVC);
app->setRootViewController(nav);

// Later, push a detail screen:
nav->pushViewController(detailVC);

// Pop back:
nav->popViewController();
```

NavigationViewController automatically displays a NavigationBar with a back button and the current view controller's title.

### TabViewController

Tabbed interface with multiple child view controllers:

```cpp
auto tabs = TabViewController::create(app);
tabs->addTab("General", generalVC);
tabs->addTab("Network", networkVC);
tabs->addTab("About", aboutVC);
app->setRootViewController(tabs);
```

### AlertViewController

Modal dialog with a message and action buttons:

```cpp
auto alert = AlertViewController::create(
    app,
    "Delete Item",                    // title
    "This cannot be undone.",         // message
    {"Cancel", "Delete"},             // button labels
    [](int buttonIndex) {             // completion handler
        if (buttonIndex == 1) { /* delete */ }
    });
app->presentViewController(alert);
```

### Modal Presentation

Any view controller can be presented modally:

```cpp
app->presentViewController(settingsVC);  // dims content, shows on top
app->dismissViewController();            // removes topmost modal
```

The content behind the modal is dimmed only when the modal's view is non-opaque; an opaque, full-screen modal covers everything, so no dimmer is added behind it.

## Events and Input

### Touch Events

When touch is enabled (`window->setTouchEnabled()`), touch events are dispatched by hit-testing the view hierarchy to find the deepest view under the touch point. That view **captures** the touch and receives all subsequent TOUCH_MOVED and TOUCH_UP events for the gesture.

On TOUCH_UP, the framework checks:
- **Swipe**: if the touch was short (<400ms) and moved far enough (>60px with a 2:1 axis ratio), a `FOCUS_EVENT_SWIPE_*` event is delivered instead.
- **Long press**: Focus defines and dispatches `FOCUS_EVENT_LONG_PRESS` but does not generate it — your platform's input layer produces it (typically after ~500ms of minimal movement). When a LONG_PRESS was delivered during a touch, TOUCH_UP_OUTSIDE is delivered on release to suppress the tap. Currently touch-only; d-pad long-press SELECT is a planned future addition.
- **Tap**: otherwise, TOUCH_UP_INSIDE or TOUCH_UP_OUTSIDE depending on whether the finger is still within the captured view's bounds.

Touch coordinates are packed into `Event.userInfo` as `(x << 16) | y`.

### Directional Navigation

When touch is not enabled, directional events (LEFT, DOWN, UP, RIGHT) navigate focus between views. Each view has a **directional affinity** that controls how its children are traversed:

- `DirectionalAffinity::Vertical`: UP/DOWN navigate between siblings (VStack sets this automatically)
- `DirectionalAffinity::Horizontal`: LEFT/RIGHT navigate between siblings (HStack sets this automatically)
- `DirectionalAffinity::None`: no automatic navigation

Events bubble up the view hierarchy until a view handles them.

### Gesture Recognizers

System-level gesture recognizers can intercept touches before they reach the view hierarchy. Register them on the Window:

```cpp
auto recognizer = std::make_shared<EdgeDragGestureRecognizer>(
    MakeRect(0, 0, 40, 800),  // activation region (left edge)
    15);                        // move threshold in pixels
recognizer->onRecognized = [](Event e) { /* edge drag started */ };
recognizer->onMoved = [](Event e) { /* drag continued */ };
recognizer->onEnded = [](Event e) { /* drag finished */ };
window->addSystemGestureRecognizer(recognizer);
```

## Text and Fonts

### Font Loading

Focus ships one font: a 5x8 fixed-width ASCII face (`BasicGlyphProvider`) compiled into the library. Until an application installs real fonts, the system font slots fall back to it, so text renders out of the box, but it's very basic. You can bring BDF fonts for real typography. The examples below use [Spleen](https://github.com/fcambus/spleen), a BSD-licensed bitmap family; Focus does not bundle it or any other BDF font.

Fonts are loaded by name from a search path:

```cpp
Font::addFontSearchPath("/system/fonts/");
auto font = Font::withName("spleen-12x24");   // tries spleen-12x24.bdp, then .bdf
```

Three system font slots provide application-wide defaults. Unset slots chain to the system font, which itself defaults to the built-in face:

```cpp
Font::setSystemFont(Font::withName("spleen-12x24"));
Font::setSystemLargeFont(Font::withName("spleen-16x32"));
Font::setSystemSmallFont(Font::withName("spleen-8x16"));

auto font = Font::systemFont();  // retrieve anywhere
```

Font instances are cached: calling `Font::withName()` twice with the same name returns the same object.

### GlyphProvider

Font rendering is abstracted behind the **GlyphProvider** interface. Focus includes six providers:

| Provider | Format | Description |
|----------|--------|-------------|
| **BDFGlyphProvider** | `.bdf` | Standard Bitmap Distribution Format. |
| **PackedFontGlyphProvider** | `.bdp` | Compact binary format (~5-10x smaller than BDF). |
| **UnifontGlyphProvider** | `.bin` | GNU Unifont (full Unicode coverage, 8x16 / 16x16). |
| **BasicGlyphProvider** | — | Hardcoded 5x8 ASCII fallback. |
| **StyledGlyphProvider** | — | Routes glyph requests to per-style providers (regular, italic, bold, bold italic). |
| **FallbackGlyphProvider** | — | Wraps a primary provider and falls back to another for missing glyphs. |

### Text Rendering

**LabelView** renders text with automatic word wrapping:

```cpp
auto label = std::make_shared<LabelView>(MakeRect(0, 0, 400, 100), "Some text");
label->setFont(font);
label->setTextAlignment(TextAlignment::Justified);
```

**CanvasView** provides lower-level text drawing alongside graphics primitives:

```cpp
canvas->setFont(font);
canvas->drawText(MakeRect(10, 10, 460, 780),
                 GrayscaleColor::Black(), 1, "Hello, world!",
                 TextAlignment::Left);
```

Soft hyphens (U+00AD) are honored throughout: invisible and zero-width within a line, they mark spots where a word may break, and when a line breaks at one, a hyphen is rendered at the line end.

### Arabic and Bidirectional Text

Focus includes Arabic contextual shaping (isolated, initial, medial, final forms) and bidirectional text rendering. These are applied automatically when Arabic characters are detected in the text. The Unicode property tables that drive bidi classification and line breaking are generated from Unicode spec files by the tools in `tools/unicodedata/`.

## Colors and Display

### Color System

Colors in Focus are `uint16_t` values. The framework provides two factory classes for different display technologies:

**GrayscaleColor**, for e-paper and monochrome displays:

```cpp
GrayscaleColor::Black()     // 0x0000
GrayscaleColor::DarkGray()  // 0x5555
GrayscaleColor::LightGray() // 0xAAAA
GrayscaleColor::White()     // 0xFFFF
```

**RGB565Color**, for 16-bit TFT/LCD displays:

```cpp
RGB565Color::Red()                    // 0xF800
RGB565Color::fromRGB(128, 200, 50)    // arbitrary color
RGB565Color::fromGrayscale(128)       // medium gray
```

Display backends are responsible for interpreting these `uint16_t` values at their native bit depth. `Color.hpp` documents the bit-shift conversions (e.g. `color >> 14` for 2-bit, `color >> 12` for 4-bit, `color >> 8` for 8-bit).

Default foreground and background colors are set globally and inherited by newly created views:

```cpp
View::SetDefaultBackgroundColor(GrayscaleColor::White());
View::SetDefaultForegroundColor(GrayscaleColor::Black());
```

### Display Interface

The **Display** abstract class defines two required rendering primitives and one optional method:

| Method | Required | Description |
|--------|----------|-------------|
| `fillRect()` | Yes | Fill a rectangle with a solid color. |
| `blitMasked()` | Yes | Apply a color where mask bits are set; leave other pixels unchanged. |
| `blitOpaque()` | No | Blit a pixel buffer (1bpp, 8bpp, or 16bpp depending on display mode). Has a slow default implementation that decodes pixels and calls `fillRect()`. Override for better performance. |

All methods accept an optional clip rectangle for efficient partial redraws.

Built-in controls use `blitMasked` for rendering: the canvas buffer acts as a shape mask, and the control's foreground color is applied at blit time. This means controls automatically render with real colors on any display type. `blitOpaque` is used by CanvasView for direct pixel-buffer rendering (e.g. photos, PDF pages).

**DisplayMode** describes the display's capability:

| Mode | Bits per pixel | Description |
|------|---------------|-------------|
| `Monochrome` | 1 | Black and white (e-paper, monochrome LCD) |
| `Grayscale` | 8 | 8-bit grayscale (one byte per pixel, 0x00-0xFF) |
| `RGB565` | 16 | 16-bit color (5 red, 6 green, 5 blue) |

Display also manages rotation (0, 90, 180, 270 degrees) and reports the current width/height accounting for rotation.

To implement a Display backend for new hardware: subclass Display, set `nativeWidth` and `nativeHeight` in your constructor, and implement `fillRect()` and `blitMasked()`. Override `blitOpaque()` for better performance if your display handles bitmap blits natively.

## Settings and Localization

### UserSettings

Key-value persistence with pluggable backends:

```cpp
// At startup, configure the storage backend:
UserSettings::setBackendFactory([](const std::string& ns) {
    return std::make_unique<MySettingsBackend>(ns);
});

// Read and write settings by namespace:
auto settings = UserSettings::withNamespace("myapp");
settings->setInt("volume", 75);
int vol = settings->getInt("volume");
settings->setString("username", "alice");
settings->setBool("dark_mode", true);
```

Focus includes `InMemorySettingsBackend` for testing. Provide your own `SettingsBackend` subclass for NVS, XML files, or any other storage.

### Localization

The Locale system provides string lookup with formatting and pluralization:

```cpp
// Setup:
Locale::addLocaleSearchPath("/locale/");
Locale::setDefaultLocale(Locale::withIdentifier("en"));
Locale::setCurrentLocale(Locale::withIdentifier("es"));

// Simple lookup (falls back to default locale if key is missing):
std::string s = _LS("save", "Save");

// Formatted string ({0}, {1}, etc.):
std::string s = _LF("page_of", "Page {0} of {1}", currentPage, totalPages);

// Pluralized string (looks up key.zero, key.one, or key.other):
std::string s = _LP("items", "{0} items", count);
```

Locale files use a simple `key=value` format:

```
# en.strings
save=Save
page_of=Page {0} of {1}
items.zero=No items
items.one=1 item
items.other={0} items
```

Changing the current locale posts a notification so views can update.

## Tasks, Timers, and Notifications

### Task

Subclass `Task` for custom background work:

```cpp
class MyTask : public Task {
public:
    bool run(std::shared_ptr<Application> app) override {
        // Do a small unit of work.
        // Return true to remove this task, false to keep running.
        return false;
    }
};

app->addTask(std::make_shared<MyTask>());
```

### Timer

Fire a callback at regular intervals:

```cpp
Timer::scheduledTimer(app, std::chrono::milliseconds(500), [](Timer& t) {
    // Called every 500ms
}, /* repeats */ true);
```

Calendar-aligned timers self-correct after clock adjustments:

```cpp
Timer::scheduledTimerForNextMinute(app, [](Timer& t) {
    // Called at the start of each minute
}, true);
```

Call `timer->invalidate()` to cancel.

### DeferredTask

Run a one-shot callback after a delay (measured in loop iterations, not wall time):

```cpp
app->addTask(std::make_shared<DeferredTask>([]() {
    // Runs after 1 loop iteration
}));
```

### NotificationCenter

For publish-subscribe messaging between objects:

```cpp
// Subscribe:
uint32_t token = NotificationCenter::shared()->addObserver(
    "DataChanged",
    [](const Notification& n) {
        // Handle notification. n.userInfo is std::any.
    },
    owner);  // optional: auto-unsubscribe when owner is destroyed

// Publish:
NotificationCenter::shared()->post("DataChanged");

// Manual unsubscribe:
NotificationCenter::shared()->removeObserver(token);
```

The owner parameter (a `shared_ptr<void>`) enables automatic cleanup: when the owner object is destroyed, the observer is silently removed. This prevents dangling callbacks on long-lived notification names.

## Accessibility

Views support an accessibility tree for test harnesses and assistive tools:

```cpp
button->accessibilityIdentifier = "save_button";  // stable programmatic ID

// These are virtual and already implemented by built-in controls:
button->accessibilityLabel();  // "Save" (human-readable)
button->accessibilityRole();   // AccessibilityRole::Button
button->accessibilityValue();  // "" (or "checked" for Checkbox, etc.)
```

Available roles: Button, StaticText, TextField, Image, Checkbox, Slider, ListItem, List, Header, Tab, ProgressBar, Adjustable.

Use `findAccessibilityElement()` to locate views by identifier for testing:

```cpp
auto view = findAccessibilityElement(window, "save_button");
```

## Code Generation Tools

Focus includes tools that generate source and data files from external specifications. These live in `tools/` within the Focus component directory.

### Unicode Data (`tools/unicodedata/`)

Three Python scripts generate the Unicode property tables used by the text rendering system:

| Script | Output | Purpose |
|--------|--------|---------|
| `generate_traits.py` | `src/text/UnicodeTraits.cpp` | Bidi class, line break, word break, mirroring flags (16-bit packed per codepoint) |
| `generate_mappings.py` | `src/text/UnicodeMappings.cpp` | Upper/lower/title case conversion and bidi mirror mappings |
| `generate_arabic.py` | `src/text/UnicodeArabicPresentation.cpp` | Arabic contextual form lookup table |

Input files (UnicodeData.txt, BidiMirroring.txt, LineBreak.txt, WordBreakProperty.txt) are included in the same directory. To regenerate after a Unicode version update:

```bash
cd components/focus/tools/unicodedata
python3 generate_traits.py
python3 generate_mappings.py
python3 generate_arabic.py
```

Note that the generated `.cpp` files for Unifont 12.1.03 are already checked into the repository.

### Unifont Converter (`tools/unifontconvert/`)

Converts GNU Unifont HEX files into a compact binary format (`unifont.bin`) with O(1) glyph lookup, consumed by `UnifontGlyphProvider`.

```bash
cd components/focus/tools/unifontconvert
python3 generate_unifont.py --output ../../path/to/fonts/unifont.bin
```

The HEX source files for Unifont 12.1.03 are included in the directory.
