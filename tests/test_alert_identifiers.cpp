/*
 * Tests for the accessibility identifiers AlertViewController assigns
 * to its internal views: alert-title, alert-message, and
 * alert-button-0 … alert-button-N-1 (index-stable, in buttonLabels order).
 *
 * These identifiers are a contract for scripted-UI drivers, which tap
 * alert buttons and assert alert wording by identifier instead of by
 * screen coordinate.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"
#include "LabelView.hpp"
#include "Application.hpp"
#include "AlertViewController.hpp"

using namespace focus;

// --- Test infrastructure (same shape as test_modal_reentrancy.cpp) ---

class AlertIdTestWindow : public Window {
public:
    AlertIdTestWindow(Size size) : Window(nullptr, size) {
        this->touchEnabled = true;
    }
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class AlertIdTestApplication : public Application {
public:
    AlertIdTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

class AlertIdTestVC : public ViewController {
public:
    AlertIdTestVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    }
};

struct AlertIdTestEnv {
    std::shared_ptr<AlertIdTestWindow> window;
    std::shared_ptr<AlertIdTestApplication> app;
};

static AlertIdTestEnv makeAlertIdTestEnv() {
    auto window = std::make_shared<AlertIdTestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<AlertIdTestApplication>(window);
    window->setApp(app);
    auto rootVC = std::make_shared<AlertIdTestVC>(app);
    app->setRootViewController(rootVC);
    return {window, app};
}

// --- Tests ---

TEST(alert_title_and_message_have_identifiers) {
    auto env = makeAlertIdTestEnv();

    auto alert = AlertViewController::create(
        env.app, "Connection Failed", "Wrong password for MyNetwork.", {"OK"});
    env.app->presentViewController(alert);

    auto title = findAccessibilityElement(alert->getView(), "alert-title");
    ASSERT_TRUE(title != nullptr);
    ASSERT_TRUE(std::dynamic_pointer_cast<LabelView>(title) != nullptr);
    ASSERT_STREQ(title->accessibilityLabel(), "Connection Failed");

    auto message = findAccessibilityElement(alert->getView(), "alert-message");
    ASSERT_TRUE(message != nullptr);
    ASSERT_TRUE(std::dynamic_pointer_cast<LabelView>(message) != nullptr);
    ASSERT_STREQ(message->accessibilityLabel(), "Wrong password for MyNetwork.");

    env.app->dismissViewController();
}

TEST(alert_buttons_have_index_stable_identifiers) {
    // Two buttons: the horizontal (HStack) layout path.
    auto env = makeAlertIdTestEnv();

    auto alert = AlertViewController::create(
        env.app, "Title", "Message", {"Cancel", "Retry"});
    env.app->presentViewController(alert);

    auto button0 = findAccessibilityElement(alert->getView(), "alert-button-0");
    ASSERT_TRUE(button0 != nullptr);
    ASSERT_TRUE(std::dynamic_pointer_cast<Button>(button0) != nullptr);
    ASSERT_STREQ(button0->accessibilityLabel(), "Cancel");

    auto button1 = findAccessibilityElement(alert->getView(), "alert-button-1");
    ASSERT_TRUE(button1 != nullptr);
    ASSERT_TRUE(std::dynamic_pointer_cast<Button>(button1) != nullptr);
    ASSERT_STREQ(button1->accessibilityLabel(), "Retry");

    // No identifier beyond the last button.
    ASSERT_TRUE(findAccessibilityElement(alert->getView(), "alert-button-2") == nullptr);

    env.app->dismissViewController();
}

TEST(alert_vertical_button_stack_has_identifiers) {
    // Three or more buttons take the vertical (VStack) layout path;
    // identifiers must be assigned there too.
    auto env = makeAlertIdTestEnv();

    auto alert = AlertViewController::create(
        env.app, "Title", "Message", {"One", "Two", "Three"});
    env.app->presentViewController(alert);

    for (int i = 0; i < 3; i++) {
        std::string identifier = "alert-button-" + std::to_string(i);
        auto button = findAccessibilityElement(alert->getView(), identifier);
        ASSERT_TRUE(button != nullptr);
        ASSERT_TRUE(std::dynamic_pointer_cast<Button>(button) != nullptr);
    }
    ASSERT_STREQ(
        findAccessibilityElement(alert->getView(), "alert-button-2")->accessibilityLabel(),
        "Three");

    env.app->dismissViewController();
}

TEST(alert_button_found_by_identifier_fires_completion) {
    // Tapping the view found by identifier must run the real button action:
    // dismiss the alert and call the completion with the matching index.
    auto env = makeAlertIdTestEnv();

    int pressedIndex = -1;
    auto alert = AlertViewController::create(
        env.app, "Title", "Message", {"Cancel", "Retry"},
        [&](int index) { pressedIndex = index; });
    env.app->presentViewController(alert);
    ASSERT_TRUE(env.app->isModalPresented());

    auto button1 = findAccessibilityElement(alert->getView(), "alert-button-1");
    ASSERT_TRUE(button1 != nullptr);

    Event event = {FOCUS_EVENT_TOUCH_UP_INSIDE, 0, 0};
    button1->handleEvent(event);

    ASSERT_EQ(pressedIndex, 1);
    ASSERT_FALSE(env.app->isModalPresented());
}
