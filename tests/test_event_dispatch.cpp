/*
 * Tests for View event dispatch: action matching, SELECT fallback
 * to TOUCH_UP_INSIDE, clipsFocus, bubbling.
 *
 * handleEvent requires a Window with an Application to invoke callbacks.
 * We create a minimal TestApplication + TestWindow to satisfy that.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"
#include "Application.hpp"

using namespace focus;

class TestWindow : public Window {
public:
    TestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class TestApplication : public Application {
public:
    TestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

struct TestEnv {
    std::shared_ptr<TestWindow> window;
    std::shared_ptr<TestApplication> app;
};

// Helper: create a Window+Application pair suitable for event dispatch tests.
static TestEnv makeTestEnv() {
    auto window = std::make_shared<TestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<TestApplication>(window);
    window->setApp(app);
    return {window, app};
}

TEST(event_dispatch_direct_action_match) {
    auto env = makeTestEnv();
    auto view = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    env.window->addSubview(view);

    bool called = false;
    view->setAction([&](Event e, std::weak_ptr<View> sender) {
        called = true;
    }, FOCUS_EVENT_SELECT);

    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    bool consumed = view->handleEvent(event);
    ASSERT_TRUE(called);
    ASSERT_TRUE(consumed);
}

TEST(event_dispatch_select_falls_back_to_touch_up_inside) {
    // If no SELECT action but TOUCH_UP_INSIDE exists, SELECT should
    // invoke the TOUCH_UP_INSIDE action (d-pad compatibility).
    auto env = makeTestEnv();
    auto view = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    env.window->addSubview(view);

    bool touchUpCalled = false;
    view->setAction([&](Event e, std::weak_ptr<View> sender) {
        touchUpCalled = true;
    }, FOCUS_EVENT_TOUCH_UP_INSIDE);

    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    bool consumed = view->handleEvent(event);
    ASSERT_TRUE(touchUpCalled);
    ASSERT_TRUE(consumed);
}

TEST(event_dispatch_no_action_bubbles) {
    auto env = makeTestEnv();
    auto parent = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    env.window->addSubview(parent);
    parent->addSubview(child);

    bool parentCalled = false;
    parent->setAction([&](Event e, std::weak_ptr<View> sender) {
        parentCalled = true;
    }, FOCUS_EVENT_SELECT);

    // Child has no action — event should bubble to parent
    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    child->handleEvent(event);
    ASSERT_TRUE(parentCalled);
}

TEST(event_dispatch_clips_focus_traps_directions) {
    auto env = makeTestEnv();
    auto parent = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    child->setClipsFocus(true);
    env.window->addSubview(parent);
    parent->addSubview(child);

    bool parentCalled = false;
    parent->setAction([&](Event e, std::weak_ptr<View> sender) {
        parentCalled = true;
    }, FOCUS_EVENT_DIRECTION_UP);

    // Directional event on clipsFocus child should NOT bubble to parent
    Event event = {FOCUS_EVENT_DIRECTION_UP, 0, 0};
    child->handleEvent(event);
    ASSERT_FALSE(parentCalled);
}

TEST(event_dispatch_expired_owner_cleans_up) {
    auto env = makeTestEnv();
    auto view = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    env.window->addSubview(view);
    bool called = false;

    {
        auto owner = std::make_shared<int>(42);
        view->setAction([&](Event e, std::weak_ptr<View> sender) {
            called = true;
        }, FOCUS_EVENT_SELECT, owner);
        // owner goes out of scope here → weak_ptr expires
    }

    // Action's owner has expired; handleEvent should remove it
    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    view->handleEvent(event);
    ASSERT_FALSE(called);
}
