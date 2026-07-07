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
#include "TextField.hpp"
#include "Application.hpp"
#include "ViewController.hpp"
#include "NavigationViewController.hpp"
#include "Window.hpp"

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

TEST(focus_engaged_false_when_window_focused) {
    auto env = makeTestEnv();
    env.window->setTouchEnabled();
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(focus_engaged_true_when_control_focused) {
    auto env = makeTestEnv();
    env.window->setTouchEnabled();
    auto button = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(button);
    ASSERT_FALSE(env.window->isFocusEngaged());  // touch windows add without focusing
    button->becomeFocused();
    ASSERT_TRUE(env.window->isFocusEngaged());
}

namespace {

// Touch-window env: latent by default.
static TestEnv makeTouchEnv() {
    auto env = makeTestEnv();
    env.window->setTouchEnabled();
    return env;
}

// x,y packed the way input tasks pack touch coordinates.
static int32_t pack(int x, int y) { return (x << 16) | y; }

class OneButtonVC : public ViewController {
public:
    OneButtonVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
        this->button = std::make_shared<Button>(MakeRect(10, 10, 100, 40), "Go");
        this->view->addSubview(this->button);
    }
    std::shared_ptr<Button> button;
};

class BareVC : public ViewController {
public:
    BareVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    }
};

}  // namespace

TEST(touch_latent_direction_summons_first_focusable) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(a);
    env.window->addSubview(b);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    // Summoning consumes the press: focus is on the FIRST focusable, not the second.
    ASSERT_TRUE(env.window->getFocusedView().lock() == a);
}

TEST(touch_engaged_direction_walks) {
    auto env = makeTouchEnv();
    auto container = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(container);
    container->addSubview(a);
    container->addSubview(b);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // summon -> a
    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // walk -> b
    ASSERT_TRUE(env.window->getFocusedView().lock() == b);
}

TEST(touch_latent_select_is_noop) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);
    bool fired = false;
    a->setAction([&](Event, std::weak_ptr<View>) { fired = true; }, FOCUS_EVENT_TOUCH_UP_INSIDE);

    env.app->generateEvent(FOCUS_EVENT_SELECT, 0);
    ASSERT_FALSE(fired);
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_engaged_select_fires_touch_up_inside_fallback) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);
    bool fired = false;
    a->setAction([&](Event, std::weak_ptr<View>) { fired = true; }, FOCUS_EVENT_TOUCH_UP_INSIDE);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // summon -> a
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0);
    ASSERT_TRUE(fired);
}

TEST(touch_latent_back_pops_navigation) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    nav->pushViewController(std::make_shared<OneButtonVC>(env.app));
    ASSERT_EQ(nav->stackDepth(), (size_t)2);
    ASSERT_FALSE(env.window->isFocusEngaged());  // tap-era push stays latent

    env.app->generateEvent(FOCUS_EVENT_BACK, 0);
    ASSERT_EQ(nav->stackDepth(), (size_t)1);
    ASSERT_FALSE(env.window->isFocusEngaged());  // BACK never summons
}

TEST(touch_engaged_back_pops_navigation) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    nav->pushViewController(std::make_shared<OneButtonVC>(env.app));

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // summon
    ASSERT_TRUE(env.window->isFocusEngaged());
    env.app->generateEvent(FOCUS_EVENT_BACK, 0);
    ASSERT_EQ(nav->stackDepth(), (size_t)1);
}

TEST(touch_window_action_consumes_direction_no_summon) {
    // The go-board contract: window-level nav actions preempt everything.
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);
    int nudges = 0;
    env.window->setAction([&](Event, std::weak_ptr<View>) { nudges++; }, FOCUS_EVENT_DIRECTION_DOWN);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    ASSERT_EQ(nudges, 2);
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_window_power_button_offered_in_both_states) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);
    int fired = 0;
    env.window->setAction([&](Event, std::weak_ptr<View>) { fired++; }, FOCUS_EVENT_POWER_BUTTON);

    env.app->generateEvent(FOCUS_EVENT_POWER_BUTTON, 0);      // latent
    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);    // summon
    ASSERT_TRUE(env.window->isFocusEngaged());
    env.app->generateEvent(FOCUS_EVENT_POWER_BUTTON, 0);      // engaged
    ASSERT_EQ(fired, 2);
}

TEST(touch_latent_summon_prefers_active_vc_content_over_nav_bar) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    auto pushed = std::make_shared<OneButtonVC>(env.app);
    nav->pushViewController(pushed);  // back button now visible in the nav bar

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == pushed->button);
}

TEST(touch_latent_summon_under_modal_stays_inside_modal) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    env.app->setRootViewController(rootVC);
    auto modal = std::make_shared<OneButtonVC>(env.app);
    env.app->presentViewController(modal);
    ASSERT_FALSE(env.window->isFocusEngaged());  // latent present stays latent

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == modal->button);
}

TEST(touch_latent_summon_under_focusable_less_modal_is_noop) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    env.app->setRootViewController(rootVC);
    env.app->presentViewController(std::make_shared<BareVC>(env.app));

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    // The domain rule: never summon behind the dimmer.
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_latent_back_under_modal_does_not_pop_nav_behind_it) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    nav->pushViewController(std::make_shared<OneButtonVC>(env.app));
    env.app->presentViewController(std::make_shared<OneButtonVC>(env.app));

    env.app->generateEvent(FOCUS_EVENT_BACK, 0);
    ASSERT_EQ(nav->stackDepth(), (size_t)2);   // clipsFocus swallowed it
    ASSERT_TRUE(env.app->isModalPresented());  // and no auto-dismiss either
}

TEST(touch_window_no_focusables_nav_events_are_safe_noops) {
    auto env = makeTouchEnv();
    env.window->addSubview(std::make_shared<View>(MakeRect(0, 0, 480, 800)));
    env.app->generateEvent(FOCUS_EVENT_DIRECTION_UP, 0);
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0);
    env.app->generateEvent(FOCUS_EVENT_BACK, 0);
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(dpad_window_dispatch_unchanged) {
    auto env = makeTestEnv();  // touchEnabled stays false
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(a);
    env.window->addSubview(b);
    ASSERT_TRUE(env.window->getFocusedView().lock() == a);  // d-pad auto-focus

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b);
}

TEST(touch_outside_focused_view_clears_engagement_and_activates_target) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 100, 100, 40), "B");
    env.window->addSubview(a);
    env.window->addSubview(b);
    bool bTouched = false;
    b->setAction([&](Event, std::weak_ptr<View>) { bTouched = true; }, FOCUS_EVENT_TOUCH_DOWN);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // summon -> a
    ASSERT_TRUE(env.window->isFocusEngaged());

    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(50, 120));  // on b
    ASSERT_FALSE(env.window->isFocusEngaged());  // cleared...
    ASSERT_TRUE(bTouched);                       // ...and the tap still landed
}

TEST(touch_inside_focused_view_keeps_engagement) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);  // summon -> a
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(50, 20));  // on a itself
    ASSERT_TRUE(env.window->isFocusEngaged());
    ASSERT_TRUE(env.window->getFocusedView().lock() == a);
}

TEST(touch_on_keyboard_keeps_text_field_focused) {
    auto env = makeTouchEnv();
    auto field = std::make_shared<TextField>(MakeRect(0, 0, 200, 40));
    env.window->addSubview(field);

    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(50, 20));  // tap field: focuses, keyboard presents
    ASSERT_TRUE(env.window->getFocusedView().lock() == field);

    // Keyboard occupies the bottom 260px of the 800px-tall window.
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(240, 700));
    ASSERT_TRUE(env.window->getFocusedView().lock() == field);  // typing continues
}

TEST(touch_outside_programmatically_focused_view_clears_it) {
    // Decision-1 delta (b): pre-existing programmatic focus in a touch window
    // is now touch-clearable.
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    env.window->addSubview(a);
    a->becomeFocused();
    ASSERT_TRUE(env.window->isFocusEngaged());

    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(300, 600));  // empty area
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(text_field_focused_by_touch_receives_select) {
    // Decision-1 delta (a): engaged + nav event dispatches to the focused
    // view instead of being offered-then-dropped.
    auto env = makeTouchEnv();
    auto field = std::make_shared<TextField>(MakeRect(0, 0, 200, 40));
    env.window->addSubview(field);
    bool fired = false;
    field->setAction([&](Event, std::weak_ptr<View>) { fired = true; },
                     FOCUS_EVENT_TOUCH_UP_INSIDE);

    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, pack(50, 20));  // tap focuses the field
    ASSERT_TRUE(env.window->getFocusedView().lock() == field);
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0);
    ASSERT_TRUE(fired);  // reached the field's TOUCH_UP_INSIDE fallback
}

TEST(accessibility_next_summons_first_previous_summons_last) {
    auto env = makeTouchEnv();
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(a);
    env.window->addSubview(b);

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == a);

    auto env2 = makeTouchEnv();
    auto c = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "C");
    auto d = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "D");
    env2.window->addSubview(c);
    env2.window->addSubview(d);
    env2.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_PREVIOUS, 0);
    ASSERT_TRUE(env2.window->getFocusedView().lock() == d);
}

TEST(accessibility_next_walks_document_order_and_wraps) {
    auto env = makeTouchEnv();
    // window { v1 { b1, h { b2, b3 } }, b4 } — nested containers, mixed depth.
    auto v1 = std::make_shared<View>(MakeRect(0, 0, 480, 400));
    auto h = std::make_shared<View>(MakeRect(0, 100, 480, 100));
    h->setDirectionalAffinity(DirectionalAffinity::Horizontal);
    auto b1 = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "1");
    auto b2 = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "2");
    auto b3 = std::make_shared<Button>(MakeRect(110, 0, 100, 40), "3");
    auto b4 = std::make_shared<Button>(MakeRect(0, 500, 100, 40), "4");
    env.window->addSubview(v1);
    v1->addSubview(b1);
    v1->addSubview(h);
    h->addSubview(b2);
    h->addSubview(b3);
    env.window->addSubview(b4);

    std::shared_ptr<View> expected[] = {b1, b2, b3, b4, b1};  // wraps at the end
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b1);
    for (int i = 1; i < 5; i++) {
        env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
        ASSERT_TRUE(env.window->getFocusedView().lock() == expected[i]);
    }
    // And PREVIOUS wraps backward off the front.
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_PREVIOUS, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b4);
}

TEST(accessibility_next_escapes_where_direction_dead_ends) {
    auto env = makeTouchEnv();
    // window { wrapper { h { b1, b2 } } } — DOWN from b2 dead-ends; NEXT must not.
    auto wrapper = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto h = std::make_shared<View>(MakeRect(0, 0, 480, 100));
    h->setDirectionalAffinity(DirectionalAffinity::Horizontal);
    auto b1 = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "1");
    auto b2 = std::make_shared<Button>(MakeRect(110, 0, 100, 40), "2");
    env.window->addSubview(wrapper);
    wrapper->addSubview(h);
    h->addSubview(b1);
    h->addSubview(b2);
    b2->becomeFocused();

    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b2);  // dead end, unchanged

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b1);  // escaped (wrapped)
}

TEST(accessibility_traversal_confined_by_modal) {
    auto env = makeTouchEnv();
    auto rootVC = std::make_shared<OneButtonVC>(env.app);
    env.app->setRootViewController(rootVC);

    // Two-button modal so NEXT has somewhere to go.
    class TwoButtonVC : public ViewController {
    public:
        TwoButtonVC(std::shared_ptr<Application> app) : ViewController(app) {}
        void createView() override {
            this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
            this->first = std::make_shared<Button>(MakeRect(10, 10, 100, 40), "1");
            this->second = std::make_shared<Button>(MakeRect(10, 60, 100, 40), "2");
            this->view->addSubview(this->first);
            this->view->addSubview(this->second);
        }
        std::shared_ptr<Button> first, second;
    };
    auto modal = std::make_shared<TwoButtonVC>(env.app);
    env.app->presentViewController(modal);

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);  // summon -> modal first
    ASSERT_TRUE(env.window->getFocusedView().lock() == modal->first);
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);  // -> modal second
    ASSERT_TRUE(env.window->getFocusedView().lock() == modal->second);
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);  // wraps INSIDE the modal
    ASSERT_TRUE(env.window->getFocusedView().lock() == modal->first);
}

TEST(dpad_window_accessibility_traversal_works) {
    auto env = makeTestEnv();  // non-touch
    auto a = std::make_shared<Button>(MakeRect(0, 0, 100, 40), "A");
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(a);
    env.window->addSubview(b);
    ASSERT_TRUE(env.window->getFocusedView().lock() == a);

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
    ASSERT_TRUE(env.window->getFocusedView().lock() == b);
}

TEST(accessibility_traversal_presents_and_dismisses_keyboard) {
    auto env = makeTouchEnv();
    auto field = std::make_shared<TextField>(MakeRect(0, 0, 200, 40));
    auto b = std::make_shared<Button>(MakeRect(0, 50, 100, 40), "B");
    env.window->addSubview(field);
    env.window->addSubview(b);

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);  // summon -> field
    ASSERT_TRUE(env.window->getFocusedView().lock() == field);
    auto onKeyboard = env.window->getViewForTouch(MakePoint(240, 700)).lock();
    ASSERT_TRUE(env.window->isKeyboardView(onKeyboard));  // keyboard presented

    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);  // keyboard has no focusables
    ASSERT_TRUE(env.window->getFocusedView().lock() == b);
    onKeyboard = env.window->getViewForTouch(MakePoint(240, 700)).lock();
    ASSERT_FALSE(env.window->isKeyboardView(onKeyboard));  // dismissed on the way out
}

TEST(touch_window_no_focusables_accessibility_traversal_is_safe_noop) {
    auto env = makeTouchEnv();
    env.window->addSubview(std::make_shared<View>(MakeRect(0, 0, 480, 800)));
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_NEXT, 0);
    env.app->generateEvent(FOCUS_EVENT_ACCESSIBILITY_PREVIOUS, 0);
    ASSERT_FALSE(env.window->isFocusEngaged());
}
