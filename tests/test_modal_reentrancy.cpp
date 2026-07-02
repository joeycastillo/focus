/*
 * Tests for re-entrant modal presentation and dismissal.
 *
 * Exercises the scenario where a modal VC's button callback (during
 * event dispatch) triggers another presentViewController or
 * dismissViewController — i.e., modal operations nested inside
 * an active generateEvent call.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"
#include "Application.hpp"
#include "AlertViewController.hpp"
#include "DeferredTask.hpp"

using namespace focus;

// --- Test infrastructure (shared with test_event_dispatch.cpp) ---

class ModalTestWindow : public Window {
public:
    ModalTestWindow(Size size) : Window(nullptr, size) {
        this->touchEnabled = true;
    }
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class ModalTestApplication : public Application {
public:
    ModalTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}

    /// Run one iteration of the task loop (process DeferredTasks etc.).
    void runOneIteration() {
        auto application = this->shared_from_this();
        for (int i = 0; i < (int)this->tasks.size(); i++) {
            if (this->tasks[i]->run(application)) {
                this->tasks.erase(this->tasks.begin() + i);
                i--;
            }
        }
    }
};

class ModalTestVC : public ViewController {
public:
    ModalTestVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    }
};

struct ModalTestEnv {
    std::shared_ptr<ModalTestWindow> window;
    std::shared_ptr<ModalTestApplication> app;
};

static ModalTestEnv makeModalTestEnv() {
    auto window = std::make_shared<ModalTestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<ModalTestApplication>(window);
    window->setApp(app);
    // Set up a root VC so the application has a base view hierarchy.
    auto rootVC = std::make_shared<ModalTestVC>(app);
    app->setRootViewController(rootVC);
    return {window, app};
}

// --- Tests ---

TEST(modal_present_from_button_callback) {
    // A button action that presents a modal VC. This is the normal
    // (non-re-entrant) case — should work before and after any fix.
    auto env = makeModalTestEnv();

    auto button = std::make_shared<Button>(MakeRect(10, 10, 100, 40), "Present");
    env.window->addSubview(button);

    bool presented = false;
    button->setAction([&](Event, std::weak_ptr<View>) {
        auto alert = AlertViewController::create(
            env.app, "Title", "Message", {"OK"},
            [](int) {});
        env.app->presentViewController(alert);
        presented = true;
    }, FOCUS_EVENT_TOUCH_UP_INSIDE);

    // Simulate: handleEvent invokes the button action
    Event event = {FOCUS_EVENT_TOUCH_UP_INSIDE, 0, 0};
    button->handleEvent(event);

    ASSERT_TRUE(presented);
    ASSERT_TRUE(env.app->isModalPresented());
}

TEST(modal_completion_presents_another_modal) {
    // An alert's completion handler presents a second alert. This is
    // the re-entrant scenario: dismissViewController runs during
    // handleEvent, and the completion handler calls presentViewController
    // before dismiss finishes unwinding.
    auto env = makeModalTestEnv();

    int completionAlertButtonIndex = -1;
    bool secondAlertPresented = false;

    // Present the first alert
    auto firstAlert = AlertViewController::create(
        env.app, "First", "First alert", {"OK"},
        [&](int index) {
            completionAlertButtonIndex = index;
            // Re-entrant: present another modal from within the
            // completion handler (which runs during dismiss).
            auto secondAlert = AlertViewController::create(
                env.app, "Second", "Second alert", {"Done"},
                [](int) {});
            env.app->presentViewController(secondAlert);
            secondAlertPresented = true;
        });
    env.app->presentViewController(firstAlert);
    ASSERT_TRUE(env.app->isModalPresented());

    // Find the OK button inside the first alert's view tree.
    // AlertVC creates buttons as subviews within a stack hierarchy.
    // We'll walk the tree to find a Button.
    std::shared_ptr<Button> okButton = nullptr;
    std::function<void(std::shared_ptr<View>)> findButton =
        [&](std::shared_ptr<View> view) {
            if (auto btn = std::dynamic_pointer_cast<Button>(view)) {
                okButton = btn;
                return;
            }
            for (auto& child : view->getSubviews()) {
                findButton(child);
                if (okButton) return;
            }
        };
    findButton(firstAlert->getView());
    ASSERT_TRUE(okButton != nullptr);

    // Simulate tapping the OK button — this triggers onButtonPressed,
    // which calls completion (which presents the second alert), then
    // dismisses the first alert.
    Event event = {FOCUS_EVENT_TOUCH_UP_INSIDE, 0, 0};
    okButton->handleEvent(event);

    // The completion handler ran
    ASSERT_EQ(completionAlertButtonIndex, 0);

    // The second alert was presented
    ASSERT_TRUE(secondAlertPresented);

    // A modal is still presented (the second alert)
    ASSERT_TRUE(env.app->isModalPresented());

    // Dismiss the second alert to clean up
    env.app->dismissViewController();
    ASSERT_FALSE(env.app->isModalPresented());
}

TEST(modal_dismiss_from_completion_then_dismiss_is_safe) {
    // The completion handler explicitly calls dismissViewController
    // itself. The AlertVC's own dismiss (in onButtonPressed) should
    // be suppressed by the A1 guard. This test verifies the two
    // fixes (A1 + A3) compose correctly.
    auto env = makeModalTestEnv();

    bool completionRan = false;

    auto alert = AlertViewController::create(
        env.app, "Test", "Self-dismiss", {"OK"},
        [&](int) {
            completionRan = true;
            // Dismiss from within the completion handler
            env.app->dismissViewController();
        });
    env.app->presentViewController(alert);
    ASSERT_TRUE(env.app->isModalPresented());

    // Find the button
    std::shared_ptr<Button> okButton = nullptr;
    std::function<void(std::shared_ptr<View>)> findButton =
        [&](std::shared_ptr<View> view) {
            if (auto btn = std::dynamic_pointer_cast<Button>(view)) {
                okButton = btn;
                return;
            }
            for (auto& child : view->getSubviews()) {
                findButton(child);
                if (okButton) return;
            }
        };
    findButton(alert->getView());
    ASSERT_TRUE(okButton != nullptr);

    // Tap the button — completion dismisses, then onButtonPressed's
    // dismiss should be a no-op (A1 guard).
    Event event = {FOCUS_EVENT_TOUCH_UP_INSIDE, 0, 0};
    okButton->handleEvent(event);

    ASSERT_TRUE(completionRan);
    // Alert should be fully dismissed, no modal left
    ASSERT_FALSE(env.app->isModalPresented());
}
