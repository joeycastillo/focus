/*
 * Tests for an Application driving several Windows, each on its own Display.
 */

#include "test_harness.hpp"
#include "Application.hpp"
#include "ViewController.hpp"
#include "AnimationSequence.hpp"
#include "Window.hpp"
#include "View.hpp"
#include "Timer.hpp"
#include "RecordingDisplay.hpp"
#include "NavigationViewController.hpp"
#include "TabViewController.hpp"
#include "AlertViewController.hpp"
#include "Button.hpp"
#include "TextField.hpp"
#include "FocusMetrics.hpp"
#include "EdgeDragGestureRecognizer.hpp"

using namespace focus;

namespace {

class TestApp : public Application {
public:
    using Application::Application;
    std::function<void(TestApp&)> onSetup;
    void setup() override { if (onSetup) onSetup(*this); }
};

struct Env {
    std::shared_ptr<RecordingDisplay> display1, display2;
    std::shared_ptr<Window> window1, window2;
    std::shared_ptr<TestApp> app;
};

Env makeEnv() {
    Env env;
    env.display1 = std::make_shared<RecordingDisplay>(160, 128);
    env.display2 = std::make_shared<RecordingDisplay>(320, 240);
    env.window1 = std::make_shared<Window>(env.display1, MakeSize(160, 128));
    env.window2 = std::make_shared<Window>(env.display2, MakeSize(320, 240));
    env.app = std::make_shared<TestApp>(env.window1);
    return env;
}

}  // namespace

TEST(get_windows_lists_constructor_window_first) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    ASSERT_EQ(env.app->getWindows().size(), (size_t)2);
    ASSERT_TRUE(env.app->getWindows()[0] == env.window1);
    ASSERT_TRUE(env.app->getWindows()[1] == env.window2);
    ASSERT_TRUE(env.app->getWindow() == env.window1);
}

TEST(added_window_is_bound_immediately) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    ASSERT_TRUE(env.window2->getApplication().lock() == env.app);
}

TEST(run_binds_every_window_before_setup) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    bool boundDuringSetup = false;
    env.app->onSetup = [&](TestApp& a) {
        boundDuringSetup = a.getWindows()[0]->getApplication().lock() == a.shared_from_this()
                        && a.getWindows()[1]->getApplication().lock() == a.shared_from_this();
    };
    env.app->run();
    ASSERT_TRUE(boundDuringSetup);
}

TEST(add_window_during_setup_binds) {
    auto env = makeEnv();
    env.app->onSetup = [&](TestApp& a) { a.addWindow(env.window2); };
    env.app->run();
    ASSERT_TRUE(env.window2->getApplication().lock() == env.app);
}

TEST(run_invalidates_every_window_and_focuses_only_the_first) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.window1->clearNeedsDisplay();
    env.window2->clearNeedsDisplay();
    env.app->run();
    ASSERT_TRUE(env.window1->needsDisplay());
    ASSERT_TRUE(env.window2->needsDisplay());
    ASSERT_TRUE(env.window1->getFocusedView().lock() == env.window1);
    ASSERT_TRUE(env.window2->getFocusedView().lock() == nullptr);
}

TEST(second_window_draws_into_its_own_display) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto view = std::make_shared<View>(MakeRect(10, 10, 20, 20));
    view->setBackgroundColor(1);
    env.window2->addSubview(view);
    env.display1->reset(); env.display2->reset();
    env.window2->draw(0, 0, env.window2->getDirtyRect());
    ASSERT_TRUE(env.display2->ops.size() > 0);
    ASSERT_EQ(env.display1->ops.size(), (size_t)0);
}

TEST(timer_scheduled_from_second_window_fires) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto view = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    env.window2->addSubview(view);
    bool fired = false;
    auto timer = view->scheduledTimer(std::chrono::milliseconds(0), [&](Timer&) { fired = true; }, false);
    ASSERT_TRUE(timer != nullptr);
    env.app->run();
    ASSERT_TRUE(fired);
}

TEST(animation_sequence_in_second_window_runs) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto view = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    env.window2->addSubview(view);
    int frames = 0; bool done = false;
    AnimationSequence seq;
    seq.animate(std::chrono::milliseconds(0), 2, [&](int) { frames++; });
    seq.start(view, [&]() { done = true; });
    env.app->run();
    ASSERT_EQ(frames, 2);
    ASSERT_TRUE(done);
}

TEST(timer_from_detached_view_is_null) {
    auto view = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    ASSERT_TRUE(view->scheduledTimer(std::chrono::milliseconds(0), [](Timer&) {}, false) == nullptr);
}

namespace {

// Test-only controller that records the size its createView() saw.
class SizeProbeVC : public ViewController {
public:
    using ViewController::ViewController;
    Size seen{0, 0};
    void host(std::shared_ptr<Window> w) { this->setWindow(w); }
protected:
    void createView() override {
        auto w = this->getWindow();
        seen = w ? w->getContentRect().size : MakeSize(0, 0);
        this->view = std::make_shared<View>(MakeRect(0, 0, seen.width, seen.height));
    }
};

}

TEST(view_controller_window_falls_back_to_key_window) {
    auto env = makeEnv();
    env.app->run();
    auto vc = std::make_shared<SizeProbeVC>(env.app);
    vc->viewWillAppear();
    ASSERT_EQ(vc->seen.width, 160);
    ASSERT_EQ(vc->seen.height, 128);
}

TEST(view_controller_sizes_from_its_hosting_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto vc = std::make_shared<SizeProbeVC>(env.app);
    vc->host(env.window2);
    vc->viewWillAppear();
    ASSERT_EQ(vc->seen.width, 320);
    ASSERT_EQ(vc->seen.height, 240);
}

TEST(attached_view_wins_over_stored_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto vc = std::make_shared<SizeProbeVC>(env.app);
    vc->host(env.window2);
    vc->viewWillAppear();
    env.window1->addSubview(vc->getView());
    ASSERT_TRUE(vc->getWindow() == env.window1);
}

TEST(root_view_controller_is_hosted_by_the_key_window) {
    auto env = makeEnv();
    env.app->run();
    auto vc = std::make_shared<SizeProbeVC>(env.app);
    env.app->setRootViewController(vc);
    ASSERT_TRUE(vc->getWindow() == env.window1);
    ASSERT_EQ(vc->seen.width, 160);
}

TEST(root_in_second_window_lands_in_that_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto vc = std::make_shared<SizeProbeVC>(env.app);
    env.window2->setRootViewController(vc);
    ASSERT_TRUE(vc->getView()->getWindow().lock() == env.window2);
    ASSERT_EQ(vc->seen.width, 320);
    ASSERT_TRUE(env.window2->getRootViewController() == vc);
    ASSERT_TRUE(env.app->activeViewController() == nullptr);   // key window has no root
}

TEST(application_presentation_forwards_to_key_window) {
    auto env = makeEnv();
    env.app->run();
    auto root = std::make_shared<SizeProbeVC>(env.app);
    auto modal = std::make_shared<SizeProbeVC>(env.app);
    env.app->setRootViewController(root);
    env.app->presentViewController(modal);
    ASSERT_TRUE(env.window1->isModalPresented());
    ASSERT_TRUE(env.window1->activeViewController() == modal);
    env.app->dismissViewController();
    ASSERT_FALSE(env.window1->isModalPresented());
}

namespace {

// Test-only controller whose view holds one focusable Button.
class FocusableVC : public ViewController {
public:
    using ViewController::ViewController;
protected:
    void createView() override {
        Size size = this->getWindow()->getContentRect().size;
        this->view = std::make_shared<View>(MakeRect(0, 0, size.width, size.height));
        this->view->addSubview(std::make_shared<Button>(MakeRect(10, 10, 60, 20), "OK"));
    }
};

}

TEST(modal_in_second_window_is_invisible_to_key_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root1 = std::make_shared<FocusableVC>(env.app);
    env.app->setRootViewController(root1);
    auto focusedBefore = env.window1->getFocusedView().lock();
    ASSERT_TRUE(focusedBefore != nullptr);
    ASSERT_TRUE(focusedBefore.get() != env.window1.get());
    auto modal = std::make_shared<FocusableVC>(env.app);
    env.window2->presentViewController(modal);
    ASSERT_TRUE(env.window2->isModalPresented());
    ASSERT_FALSE(env.app->isModalPresented());
    ASSERT_TRUE(env.window1->getFocusedView().lock() == focusedBefore);
    ASSERT_TRUE(modal->getView()->getWindow().lock() == env.window2);
    env.window2->dismissViewController();
    ASSERT_FALSE(env.window2->isModalPresented());
    ASSERT_TRUE(env.window1->getFocusedView().lock() == focusedBefore);
}

TEST(modal_in_second_window_sizes_from_that_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto modal = std::make_shared<SizeProbeVC>(env.app);
    env.window2->presentViewController(modal);
    ASSERT_EQ(modal->seen.width, 320);
    ASSERT_EQ(modal->seen.height, 240);
}

TEST(view_controller_presents_and_dismisses_in_its_own_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root2 = std::make_shared<SizeProbeVC>(env.app);
    env.window2->setRootViewController(root2);
    auto modal = std::make_shared<SizeProbeVC>(env.app);
    root2->presentViewController(modal);
    ASSERT_TRUE(env.window2->activeViewController() == modal);
    modal->dismissViewController();
    ASSERT_FALSE(env.window2->isModalPresented());
    ASSERT_TRUE(env.window2->activeViewController() == root2);
}

TEST(alert_in_second_window_dismisses_from_second_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root1 = std::make_shared<SizeProbeVC>(env.app);
    env.app->setRootViewController(root1);
    int pressedIndex = -1;
    auto alert = AlertViewController::create(
        env.app, "Title", "Message", {"Cancel", "Retry"},
        [&](int index) { pressedIndex = index; });
    env.window2->presentViewController(alert);
    ASSERT_TRUE(env.window2->isModalPresented());
    ASSERT_FALSE(env.app->isModalPresented());

    auto button1 = findAccessibilityElement(alert->getView(), "alert-button-1");
    ASSERT_TRUE(button1 != nullptr);
    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    button1->handleEvent(event);

    ASSERT_EQ(pressedIndex, 1);
    ASSERT_FALSE(env.window2->isModalPresented());
    ASSERT_FALSE(env.app->isModalPresented());
    ASSERT_TRUE(env.app->activeViewController() == root1);
}

TEST(navigation_controller_and_child_size_from_hosting_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto child = std::make_shared<SizeProbeVC>(env.app);
    auto nav = NavigationViewController::create(env.app, child);
    env.window2->setRootViewController(nav);
    ASSERT_EQ(nav->getView()->getFrame().size.width, 320);
    ASSERT_EQ(nav->getView()->getFrame().size.height, 240);
    ASSERT_EQ(child->seen.width, 320);
    auto pushed = std::make_shared<SizeProbeVC>(env.app);
    nav->pushViewController(pushed);
    ASSERT_EQ(pushed->seen.width, 320);
}

TEST(tab_controller_and_child_size_from_hosting_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto a = std::make_shared<SizeProbeVC>(env.app);
    auto b = std::make_shared<SizeProbeVC>(env.app);
    auto tabs = TabViewController::create(env.app);
    tabs->addTab("a", a);
    tabs->addTab("b", b);
    env.window2->setRootViewController(tabs);
    ASSERT_EQ(tabs->getView()->getFrame().size.width, 320);
    ASSERT_EQ(a->seen.width, 320);
    tabs->selectTab(1);
    ASSERT_EQ(b->seen.width, 320);
}

static int32_t packXY(int x, int y) { return (x << 16) | y; }

TEST(positional_event_targets_named_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.window2->setTouchEnabled();
    env.app->run();
    auto view = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    env.window2->addSubview(view);
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5), env.window2);
    ASSERT_TRUE(env.window2->getCapturedTouchView().lock() == view);
    ASSERT_TRUE(env.window1->getCapturedTouchView().lock() == nullptr);
}

TEST(two_argument_generate_event_targets_key_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.window1->setTouchEnabled();
    env.app->run();
    auto view = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    env.window1->addSubview(view);
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5));
    ASSERT_TRUE(env.window1->getCapturedTouchView().lock() == view);
}

TEST(touch_state_is_per_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.window1->setTouchEnabled();
    env.window2->setTouchEnabled();
    env.app->run();
    auto v1 = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    auto v2 = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    env.window1->addSubview(v1);
    env.window2->addSubview(v2);
    int32_t up1 = 0, up2 = 0;
    v1->setAction([&](Event e, std::weak_ptr<View>) { up1 = e.type; }, FOCUS_EVENT_TOUCH_UP_INSIDE);
    v1->setAction([&](Event e, std::weak_ptr<View>) { up1 = e.type; }, FOCUS_EVENT_TOUCH_UP_OUTSIDE);
    v2->setAction([&](Event e, std::weak_ptr<View>) { up2 = e.type; }, FOCUS_EVENT_TOUCH_UP_INSIDE);
    v2->setAction([&](Event e, std::weak_ptr<View>) { up2 = e.type; }, FOCUS_EVENT_TOUCH_UP_OUTSIDE);
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5), env.window1);
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5), env.window2);
    env.app->generateEvent(FOCUS_EVENT_LONG_PRESS, packXY(5, 5), env.window2);   // only window2 long-pressed
    env.app->generateEvent(FOCUS_EVENT_TOUCH_UP, packXY(5, 5), env.window1);
    env.app->generateEvent(FOCUS_EVENT_TOUCH_UP, packXY(5, 5), env.window2);
    ASSERT_EQ(up1, FOCUS_EVENT_TOUCH_UP_INSIDE);
    ASSERT_EQ(up2, FOCUS_EVENT_TOUCH_UP_OUTSIDE);
}

TEST(make_key_window_redirects_non_positional_events) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto b1 = std::make_shared<Button>(MakeRect(0, 0, 50, 20), "1");
    auto b2 = std::make_shared<Button>(MakeRect(0, 0, 50, 20), "2");
    env.window1->addSubview(b1);
    env.window2->addSubview(b2);
    bool hit1 = false, hit2 = false;
    b1->setAction([&](Event, std::weak_ptr<View>) { hit1 = true; }, FOCUS_EVENT_SELECT);
    b2->setAction([&](Event, std::weak_ptr<View>) { hit2 = true; }, FOCUS_EVENT_SELECT);
    env.app->makeKeyWindow(env.window2);
    ASSERT_TRUE(env.app->getKeyWindow() == env.window2);
    ASSERT_TRUE(env.app->getWindow() == env.window2);
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0);
    ASSERT_FALSE(hit1);
    ASSERT_TRUE(hit2);
}

TEST(make_key_window_ignores_unregistered_window) {
    auto env = makeEnv();
    env.app->run();
    env.app->makeKeyWindow(env.window2);   // never added
    ASSERT_TRUE(env.app->getKeyWindow() == env.window1);
}

TEST(gesture_recognizer_state_is_per_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.window1->setTouchEnabled();
    env.window2->setTouchEnabled();
    env.app->run();
    auto v1 = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    auto v2 = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    env.window1->addSubview(v1);
    env.window2->addSubview(v2);
    auto edge = std::make_shared<EdgeDragGestureRecognizer>(MakeRect(0, 0, 20, 240), 15);
    bool recognized = false;
    edge->onRecognized = [&](Event) { recognized = true; };
    env.window2->addSystemGestureRecognizer(edge);

    // window2's touch starts in the edge region, so it is buffered, not captured.
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5), env.window2);
    // The same touch on window1, which has no recognizers, captures normally.
    env.app->generateEvent(FOCUS_EVENT_TOUCH_DOWN, packXY(5, 5), env.window1);
    ASSERT_TRUE(env.window1->getCapturedTouchView().lock() == v1);
    ASSERT_TRUE(env.window2->getCapturedTouchView().lock() == nullptr);

    // window1's touch did not disturb window2's pending recognition: a drag
    // past the threshold on window2 is still recognized, and still captures nothing.
    env.app->generateEvent(FOCUS_EVENT_TOUCH_MOVED, packXY(30, 5), env.window2);
    ASSERT_TRUE(recognized);
    ASSERT_TRUE(env.window2->getCapturedTouchView().lock() == nullptr);
    ASSERT_TRUE(env.window1->getCapturedTouchView().lock() == v1);
}

TEST(generate_event_with_null_window_is_ignored) {
    auto env = makeEnv();
    env.app->run();
    auto before = env.window1->getFocusedView().lock();
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0, nullptr);
    ASSERT_TRUE(env.window1->getFocusedView().lock() == before);
    ASSERT_TRUE(env.app->getKeyWindow() == env.window1);
}

TEST(keyboard_height_comes_from_metrics) {
    struct RestoreMetrics {
        ~RestoreMetrics() { FocusMetrics::set(FocusMetrics::Standard); }
    } restore;
    FocusMetrics m = FocusMetrics::Standard;
    m.keyboardHeight = 100;
    FocusMetrics::set(m);

    auto env = makeEnv();
    env.window1->setTouchEnabled();
    env.app->run();
    auto field = std::make_shared<TextField>(MakeRect(0, 0, 100, 20));
    env.window1->addSubview(field);
    field->becomeFocused();  // focusing a text field presents the keyboard

    bool found = false;
    for (auto& sv : env.window1->getSubviews()) {
        if (env.window1->isKeyboardView(sv)) {
            found = true;
            ASSERT_EQ(sv->getFrame().size.height, 100);
        }
    }
    ASSERT_TRUE(found);
}

TEST(add_window_ignores_null) {
    auto env = makeEnv();
    env.app->addWindow(nullptr);
    ASSERT_EQ(env.app->getWindows().size(), (size_t)1);
    env.app->run();
    ASSERT_TRUE(env.app->getKeyWindow() == env.window1);
}

TEST(generate_event_ignores_unregistered_window) {
    auto env = makeEnv();
    env.app->run();
    auto display3 = std::make_shared<RecordingDisplay>(100, 100);
    auto stray = std::make_shared<Window>(display3, MakeSize(100, 100));   // never added
    stray->setTouchEnabled();
    stray->addSubview(std::make_shared<Button>(MakeRect(0, 0, 50, 20), "S"));
    auto before = env.window1->getFocusedView().lock();
    auto strayBefore = stray->getFocusedView().lock();   // setTouchEnabled() focuses the window itself
    ASSERT_TRUE(strayBefore == stray);
    env.app->generateEvent(FOCUS_EVENT_SELECT, 0, stray);
    // A directional event would summon focus onto the button if it were dispatched.
    env.app->generateEvent(FOCUS_EVENT_DIRECTION_DOWN, 0, stray);
    ASSERT_TRUE(env.window1->getFocusedView().lock() == before);
    ASSERT_TRUE(stray->getFocusedView().lock() == strayBefore);
}

TEST(view_controller_generate_event_targets_its_own_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto vc1 = std::make_shared<FocusableVC>(env.app);
    auto vc2 = std::make_shared<FocusableVC>(env.app);
    env.app->setRootViewController(vc1);
    env.window2->setRootViewController(vc2);
    auto b1 = vc1->getView()->getSubviews()[0];
    auto b2 = vc2->getView()->getSubviews()[0];
    ASSERT_TRUE(env.window1->getFocusedView().lock() == b1);
    ASSERT_TRUE(env.window2->getFocusedView().lock() == b2);
    bool hit1 = false, hit2 = false;
    b1->setAction([&](Event, std::weak_ptr<View>) { hit1 = true; }, FOCUS_EVENT_SELECT);
    b2->setAction([&](Event, std::weak_ptr<View>) { hit2 = true; }, FOCUS_EVENT_SELECT);
    vc2->generateEvent(FOCUS_EVENT_SELECT);
    ASSERT_FALSE(hit1);
    ASSERT_TRUE(hit2);
}

TEST(make_key_window_focuses_an_unfocused_dpad_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    ASSERT_TRUE(env.window2->getFocusedView().lock() == nullptr);
    env.app->makeKeyWindow(env.window2);
    ASSERT_TRUE(env.window2->getFocusedView().lock() == env.window2);
}

namespace {

bool isInside(const std::shared_ptr<View>& view, const std::shared_ptr<View>& ancestor) {
    for (View* v = view.get(); v; v = v->getSuperview()) {
        if (v == ancestor.get()) return true;
    }
    return false;
}

}

TEST(modals_in_two_windows_are_independent) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root1 = std::make_shared<FocusableVC>(env.app);
    auto root2 = std::make_shared<FocusableVC>(env.app);
    env.app->setRootViewController(root1);
    env.window2->setRootViewController(root2);
    auto modal1 = std::make_shared<FocusableVC>(env.app);
    auto modal2 = std::make_shared<FocusableVC>(env.app);
    env.app->presentViewController(modal1);
    env.window2->presentViewController(modal2);
    ASSERT_TRUE(env.window1->activeViewController() == modal1);
    ASSERT_TRUE(env.window2->activeViewController() == modal2);

    env.app->dismissViewController();
    ASSERT_FALSE(env.window1->isModalPresented());
    ASSERT_TRUE(env.window2->isModalPresented());
    ASSERT_TRUE(env.window2->activeViewController() == modal2);
    auto focused2 = env.window2->getFocusedView().lock();
    ASSERT_TRUE(focused2 != nullptr);
    ASSERT_TRUE(isInside(focused2, modal2->getView()));

    env.window2->dismissViewController();
    ASSERT_FALSE(env.window1->isModalPresented());
    ASSERT_FALSE(env.window2->isModalPresented());
    ASSERT_TRUE(env.window2->activeViewController() == root2);
    auto focused1 = env.window1->getFocusedView().lock();
    ASSERT_TRUE(focused1 != nullptr);
    ASSERT_TRUE(isInside(focused1, root1->getView()));
}

TEST(dismiss_all_on_non_key_window_leaves_key_window_alone) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root1 = std::make_shared<FocusableVC>(env.app);
    auto root2 = std::make_shared<FocusableVC>(env.app);
    env.app->setRootViewController(root1);
    env.window2->setRootViewController(root2);
    auto modal1 = std::make_shared<FocusableVC>(env.app);
    env.app->presentViewController(modal1);
    env.window2->presentViewController(std::make_shared<FocusableVC>(env.app));
    env.window2->presentViewController(std::make_shared<FocusableVC>(env.app));

    env.window2->dismissAllViewControllers();
    ASSERT_FALSE(env.window2->isModalPresented());
    ASSERT_TRUE(env.window2->activeViewController() == root2);
    ASSERT_TRUE(env.window1->isModalPresented());
    ASSERT_TRUE(env.window1->activeViewController() == modal1);
}

TEST(set_root_on_second_window_under_key_window_modal) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto root1 = std::make_shared<FocusableVC>(env.app);
    env.app->setRootViewController(root1);
    auto modal1 = std::make_shared<FocusableVC>(env.app);
    env.app->presentViewController(modal1);
    auto focusedModal = env.window1->getFocusedView().lock();
    ASSERT_TRUE(isInside(focusedModal, modal1->getView()));

    auto root2 = std::make_shared<FocusableVC>(env.app);
    env.window2->setRootViewController(root2);
    ASSERT_TRUE(env.window1->isModalPresented());
    ASSERT_TRUE(env.window1->activeViewController() == modal1);
    ASSERT_TRUE(env.window1->getFocusedView().lock() == focusedModal);
    ASSERT_TRUE(root2->getView()->getWindow().lock() == env.window2);
}

TEST(app_present_after_make_key_window_targets_new_key_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    env.app->makeKeyWindow(env.window2);
    auto modal = std::make_shared<SizeProbeVC>(env.app);
    env.app->presentViewController(modal);
    ASSERT_TRUE(env.window2->isModalPresented());
    ASSERT_FALSE(env.window1->isModalPresented());
    ASSERT_EQ(modal->seen.width, 320);
}

TEST(alert_in_second_window_sizes_from_that_window) {
    auto env = makeEnv();
    env.app->addWindow(env.window2);
    env.app->run();
    auto alert = AlertViewController::create(env.app, "Title", "Message", {"OK"}, [](int) {});
    env.window2->presentViewController(alert);
    // The alert's root view covers the whole window; its box is 85% of the window width.
    Rect frame = alert->getView()->getFrame();
    ASSERT_EQ(frame.size.width, 320);
    ASSERT_EQ(frame.size.height, 240);
    auto box = alert->getView()->getSubviews()[0];
    ASSERT_EQ(box->getFrame().size.width, 320 * 85 / 100);
}
