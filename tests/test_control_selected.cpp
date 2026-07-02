/*
 * Tests for Control::selected state and Button's visual response
 * to selection (color swap, focus interaction, accessibility).
 */

#include "test_harness.hpp"
#include "Control.hpp"
#include "Button.hpp"
#include "Checkbox.hpp"
#include "CircularButton.hpp"
#include "Application.hpp"
#include "CanvasView.hpp"

using namespace focus;

// --- Minimal test environment for focus tests ---

class SelTestWindow : public Window {
public:
    SelTestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class SelTestApp : public Application {
public:
    SelTestApp(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

struct SelTestEnv {
    std::shared_ptr<SelTestWindow> window;
    std::shared_ptr<SelTestApp> app;
};

static SelTestEnv makeSelTestEnv() {
    auto window = std::make_shared<SelTestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<SelTestApp>(window);
    window->setApp(app);
    return {window, app};
}

// --- Control base class tests ---

TEST(control_selected_defaults_to_false) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");
    ASSERT_FALSE(button->isSelected());
}

TEST(control_set_selected_roundtrip) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");
    button->setSelected(true);
    ASSERT_TRUE(button->isSelected());
    button->setSelected(false);
    ASSERT_FALSE(button->isSelected());
}

// --- Button visual support tests ---

TEST(button_set_selected_swaps_colors) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");
    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();
    ASSERT_NE(origBg, origFg);

    button->setSelected(true);

    ASSERT_EQ(button->getBackgroundColor(), origFg);
    ASSERT_EQ(button->getForegroundColor(), origBg);
}

TEST(button_unselect_restores_colors) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");
    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();

    button->setSelected(true);
    button->setSelected(false);

    ASSERT_EQ(button->getBackgroundColor(), origBg);
    ASSERT_EQ(button->getForegroundColor(), origFg);
}

TEST(button_focus_when_selected_no_double_swap) {
    auto env = makeSelTestEnv();
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");

    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();

    env.window->addSubview(button);
    // Resign auto-focus to get clean unfocused state
    button->resignFocus();

    // Select first (swaps colors: highlighted = true)
    button->setSelected(true);
    ASSERT_EQ(button->getBackgroundColor(), origFg);

    // Focus should NOT swap again — already highlighted
    button->becomeFocused();
    ASSERT_EQ(button->getBackgroundColor(), origFg);
    ASSERT_EQ(button->getForegroundColor(), origBg);
}

TEST(button_focus_when_unselected_swaps_normally) {
    auto env = makeSelTestEnv();
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");

    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();

    env.window->addSubview(button);

    // addSubview auto-focuses the first focusable control — colors should be swapped
    ASSERT_EQ(button->getBackgroundColor(), origFg);
    ASSERT_EQ(button->getForegroundColor(), origBg);
}

TEST(button_resign_focus_when_selected_no_swap) {
    auto env = makeSelTestEnv();
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");

    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();

    env.window->addSubview(button);
    // Resign auto-focus to get clean state
    button->resignFocus();

    // Select and focus
    button->setSelected(true);
    button->becomeFocused();

    // Resign focus — should NOT swap (still selected = still highlighted)
    button->resignFocus();
    ASSERT_EQ(button->getBackgroundColor(), origFg);
    ASSERT_EQ(button->getForegroundColor(), origBg);
}

TEST(button_accessibility_value_when_selected) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Test");
    ASSERT_STREQ(button->accessibilityValue(), "");

    button->setSelected(true);
    ASSERT_STREQ(button->accessibilityValue(), "selected");

    button->setSelected(false);
    ASSERT_STREQ(button->accessibilityValue(), "");
}

// --- Checkbox migration smoke test ---

TEST(checkbox_uses_selected_api) {
    auto checkbox = std::make_shared<Checkbox>(MakeRect(0, 0, 200, 40), "Option");
    ASSERT_FALSE(checkbox->isSelected());
    checkbox->setSelected(true);
    ASSERT_TRUE(checkbox->isSelected());
}

// --- CanvasView::drawMask tests ---

TEST(canvas_draw_mask_sets_bits) {
    auto canvas = std::make_shared<CanvasView>(MakeRect(0, 0, 8, 2));
    canvas->clear(0);

    const uint8_t mask[] = { 0xF0, 0x90 };
    int maskRowBytes = 1;

    canvas->drawMask(0, 0, 4, 2, mask, maskRowBytes, 1);

    const uint8_t* buf = canvas->getBufferData();
    ASSERT_EQ(buf[0], 0xF0);
    ASSERT_EQ(buf[1], 0x90);
}

TEST(canvas_draw_mask_clears_bits) {
    auto canvas = std::make_shared<CanvasView>(MakeRect(0, 0, 8, 1));
    canvas->clear(1);

    const uint8_t mask[] = { 0xAA };
    canvas->drawMask(0, 0, 8, 1, mask, 1, 0);

    const uint8_t* buf = canvas->getBufferData();
    ASSERT_EQ(buf[0], 0x55);
}

TEST(canvas_draw_mask_with_offset) {
    auto canvas = std::make_shared<CanvasView>(MakeRect(0, 0, 16, 1));
    canvas->clear(0);

    const uint8_t mask[] = { 0xF0 };
    canvas->drawMask(4, 0, 4, 1, mask, 1, 1);

    const uint8_t* buf = canvas->getBufferData();
    ASSERT_EQ(buf[0], 0x0F);
    ASSERT_EQ(buf[1], 0x00);
}

// --- State-keyed content tests ---

TEST(button_title_from_constructor) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Click Me");
    ASSERT_STREQ(button->getTitle(), "Click Me");
}

TEST(button_set_title_normal) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Original");
    button->setTitle("Updated");
    ASSERT_STREQ(button->getTitle(), "Updated");
}

TEST(button_title_resolves_by_state) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Off");
    button->setTitle("On", ControlState::Selected);

    ASSERT_STREQ(button->getTitle(), "Off");

    button->setSelected(true);
    ASSERT_STREQ(button->getTitle(), "On");

    button->setSelected(false);
    ASSERT_STREQ(button->getTitle(), "Off");
}

TEST(button_title_falls_back_to_normal) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Always");
    button->setSelected(true);
    ASSERT_STREQ(button->getTitle(), "Always");
}

TEST(button_image_resolves_by_state) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "");

    const uint8_t normalIcon[] = { 0xFF };
    const uint8_t selectedIcon[] = { 0xAA };

    button->setImage(normalIcon, MakeSize(8, 1));
    button->setImage(selectedIcon, MakeSize(8, 1), ControlState::Selected);

    ASSERT_EQ(button->getImage(), normalIcon);

    button->setSelected(true);
    ASSERT_EQ(button->getImage(), selectedIcon);

    button->setSelected(false);
    ASSERT_EQ(button->getImage(), normalIcon);
}

TEST(button_image_falls_back_to_normal) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "");

    const uint8_t icon[] = { 0xFF };
    button->setImage(icon, MakeSize(8, 1));

    button->setSelected(true);
    ASSERT_EQ(button->getImage(), icon);
}

TEST(button_no_image_returns_null) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Text Only");
    ASSERT_EQ(button->getImage(), nullptr);
}

TEST(button_accessibility_label_uses_resolved_title) {
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "Normal");
    button->setTitle("Selected", ControlState::Selected);

    ASSERT_STREQ(button->accessibilityLabel(), "Normal");

    button->setSelected(true);
    ASSERT_STREQ(button->accessibilityLabel(), "Selected");
}

// --- CircularButton tests ---

TEST(circular_button_renders_without_crash) {
    auto button = std::make_shared<CircularButton>(MakeRect(0, 0, 48, 48));
    const uint8_t icon[] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF };
    button->setImage(icon, MakeSize(8, 8));
    ASSERT_STREQ(button->getTitle(), "");
    ASSERT_EQ(button->getImage(), icon);
}

TEST(circular_button_inherits_state_keyed_content) {
    auto button = std::make_shared<CircularButton>(MakeRect(0, 0, 48, 48));
    const uint8_t iconA[] = { 0xFF };
    const uint8_t iconB[] = { 0xAA };
    button->setImage(iconA, MakeSize(8, 1));
    button->setImage(iconB, MakeSize(8, 1), ControlState::Selected);

    ASSERT_EQ(button->getImage(), iconA);
    button->setSelected(true);
    ASSERT_EQ(button->getImage(), iconB);
}

TEST(circular_button_selected_swaps_colors) {
    auto button = std::make_shared<CircularButton>(MakeRect(0, 0, 48, 48));
    uint16_t origBg = button->getBackgroundColor();
    uint16_t origFg = button->getForegroundColor();

    button->setSelected(true);
    ASSERT_EQ(button->getBackgroundColor(), origFg);
    ASSERT_EQ(button->getForegroundColor(), origBg);
}
