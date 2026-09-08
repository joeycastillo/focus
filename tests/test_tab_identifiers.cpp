/*
 * Tests for the accessibility identifiers TabViewController assigns to its
 * tab bar: tab-bar on the strip and tab-item-0 through tab-item-N-1 on the
 * items (index-stable, in addTab() order), plus TabItem's label, role, and
 * value.
 *
 * These identifiers are a contract for scripted-UI drivers, which select a
 * tab and assert which tab is active by identifier instead of by coordinate.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "StackView.hpp"
#include "TabItem.hpp"
#include "Application.hpp"
#include "TabViewController.hpp"

using namespace focus;

// --- Test infrastructure (same shape as test_alert_identifiers.cpp) ---

class TabIdTestWindow : public Window {
public:
    TabIdTestWindow(Size size) : Window(nullptr, size) {
        this->touchEnabled = true;
    }
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class TabIdTestApplication : public Application {
public:
    TabIdTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

// Serves as both the root controller and the content of every tab.
class TabIdTestVC : public ViewController {
public:
    TabIdTestVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    }
};

struct TabIdTestEnv {
    std::shared_ptr<TabIdTestWindow> window;
    std::shared_ptr<TabIdTestApplication> app;
};

static TabIdTestEnv makeTabIdTestEnv() {
    auto window = std::make_shared<TabIdTestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<TabIdTestApplication>(window);
    window->setApp(app);
    auto rootVC = std::make_shared<TabIdTestVC>(app);
    app->setRootViewController(rootVC);
    return {window, app};
}

// Three tabs, presented so the controller has a live view and window.
static std::shared_ptr<TabViewController> presentThreeTabs(TabIdTestEnv& env) {
    auto tabVC = TabViewController::create(env.app);
    tabVC->addTab("Book", std::make_shared<TabIdTestVC>(env.app));
    tabVC->addTab("Contents", std::make_shared<TabIdTestVC>(env.app));
    tabVC->addTab("Font", std::make_shared<TabIdTestVC>(env.app));
    env.app->presentViewController(tabVC);
    return tabVC;
}

// --- Tests ---

TEST(tab_items_have_index_stable_identifiers) {
    auto env = makeTabIdTestEnv();
    auto tabVC = presentThreeTabs(env);

    const char* labels[] = {"Book", "Contents", "Font"};
    for (int i = 0; i < 3; i++) {
        std::string identifier = "tab-item-" + std::to_string(i);
        auto item = findAccessibilityElement(tabVC->getView(), identifier);
        ASSERT_TRUE(item != nullptr);
        ASSERT_TRUE(std::dynamic_pointer_cast<TabItem>(item) != nullptr);
        ASSERT_STREQ(item->accessibilityLabel(), labels[i]);
        ASSERT_TRUE(item->accessibilityRole() == AccessibilityRole::Tab);
    }

    // No identifier beyond the last tab.
    ASSERT_TRUE(findAccessibilityElement(tabVC->getView(), "tab-item-3") == nullptr);

    env.app->dismissViewController();
}

TEST(tab_item_value_follows_selection) {
    auto env = makeTabIdTestEnv();
    auto tabVC = presentThreeTabs(env);

    // The first tab added is selected by default.
    auto item0 = findAccessibilityElement(tabVC->getView(), "tab-item-0");
    auto item1 = findAccessibilityElement(tabVC->getView(), "tab-item-1");
    auto item2 = findAccessibilityElement(tabVC->getView(), "tab-item-2");
    ASSERT_TRUE(item0 != nullptr && item1 != nullptr && item2 != nullptr);
    ASSERT_STREQ(item0->accessibilityValue(), "selected");
    ASSERT_STREQ(item1->accessibilityValue(), "");
    ASSERT_STREQ(item2->accessibilityValue(), "");

    tabVC->selectTab(2);

    // Look the items up again in case selecting a tab rebuilt the bar.
    item0 = findAccessibilityElement(tabVC->getView(), "tab-item-0");
    item1 = findAccessibilityElement(tabVC->getView(), "tab-item-1");
    item2 = findAccessibilityElement(tabVC->getView(), "tab-item-2");
    ASSERT_TRUE(item0 != nullptr && item1 != nullptr && item2 != nullptr);
    ASSERT_STREQ(item0->accessibilityValue(), "");
    ASSERT_STREQ(item1->accessibilityValue(), "");
    ASSERT_STREQ(item2->accessibilityValue(), "selected");

    env.app->dismissViewController();
}

TEST(tab_item_found_by_identifier_selects_tab) {
    // Tapping the view found by identifier must run the real tab action.
    auto env = makeTabIdTestEnv();
    auto tabVC = presentThreeTabs(env);
    ASSERT_EQ(tabVC->getSelectedTab(), (size_t)0);

    auto item1 = findAccessibilityElement(tabVC->getView(), "tab-item-1");
    ASSERT_TRUE(item1 != nullptr);

    Event event = {FOCUS_EVENT_TOUCH_UP_INSIDE, 0, 0};
    item1->handleEvent(event);

    ASSERT_EQ(tabVC->getSelectedTab(), (size_t)1);
    ASSERT_STREQ(item1->accessibilityValue(), "selected");

    env.app->dismissViewController();
}

TEST(tab_added_after_view_exists_gets_next_identifier) {
    // addTab() on a live controller rebuilds the bar; identifiers must
    // stay index-stable across that rebuild.
    auto env = makeTabIdTestEnv();
    auto tabVC = presentThreeTabs(env);

    tabVC->addTab("Search", std::make_shared<TabIdTestVC>(env.app));

    const char* labels[] = {"Book", "Contents", "Font", "Search"};
    for (int i = 0; i < 4; i++) {
        std::string identifier = "tab-item-" + std::to_string(i);
        auto item = findAccessibilityElement(tabVC->getView(), identifier);
        ASSERT_TRUE(item != nullptr);
        ASSERT_STREQ(item->accessibilityLabel(), labels[i]);
    }
    ASSERT_TRUE(findAccessibilityElement(tabVC->getView(), "tab-item-4") == nullptr);

    env.app->dismissViewController();
}

TEST(tab_bar_has_identifier) {
    auto env = makeTabIdTestEnv();
    auto tabVC = presentThreeTabs(env);

    auto bar = findAccessibilityElement(tabVC->getView(), "tab-bar");
    ASSERT_TRUE(bar != nullptr);
    ASSERT_TRUE(std::dynamic_pointer_cast<HStack>(bar) != nullptr);

    // The items live under the strip.
    ASSERT_TRUE(findAccessibilityElement(bar, "tab-item-0") != nullptr);

    env.app->dismissViewController();
}
