/*
 * Tests for focus placement on navigation push/pop.
 *
 * In non-touch (d-pad) mode, a push or pop must leave something focused so the
 * user can keep navigating. When the newly shown view controller's content has
 * no focusable views, focus must fall back to the navigation bar's back button
 * rather than being lost. Regression coverage for the fix in 8d9e376, which
 * originally searched only the content subtree and so left focus empty when the
 * content had nothing focusable.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"
#include "Application.hpp"
#include "ViewController.hpp"
#include "Window.hpp"
#include "NavigationViewController.hpp"
#include "CollectionViewController.hpp"
#include "CollectionViewCell.hpp"

using namespace focus;

namespace {

// Non-touch window (d-pad mode): touchEnabled stays false by default.
class NavTestWindow : public Window {
public:
    NavTestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) { this->application = app; }
};

class NavTestApplication : public Application {
public:
    NavTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

// A view controller whose content has one focusable button.
class ButtonContentVC : public ViewController {
public:
    ButtonContentVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 160, 128));
        this->button = std::make_shared<Button>(MakeRect(0, 0, 160, 20), "Go");
        this->view->addSubview(this->button);
    }
    std::shared_ptr<Button> button;
};

// A view controller whose content has NO focusable views (mirrors the About
// screen after its redundant back button was removed).
class BareContentVC : public ViewController {
public:
    BareContentVC(std::shared_ptr<Application> app) : ViewController(app) {}
    void createView() override {
        this->view = std::make_shared<View>(MakeRect(0, 0, 160, 128));
    }
};

struct NavTestEnv {
    std::shared_ptr<NavTestWindow> window;
    std::shared_ptr<NavTestApplication> app;
};

static NavTestEnv makeNavTestEnv() {
    auto window = std::make_shared<NavTestWindow>(MakeSize(160, 128));
    auto app = std::make_shared<NavTestApplication>(window);
    window->setApp(app);
    return {window, app};
}

// The navigation bar is added before the content area, so it is the first
// subview of the NavigationViewController's view.
static std::shared_ptr<View> navBarOf(std::shared_ptr<NavigationViewController> nav) {
    return nav->getView()->getSubviews().at(0);
}

}  // namespace

TEST(navigation_push_contentless_vc_focuses_back_button) {
    auto env = makeNavTestEnv();
    ASSERT_FALSE(env.window->isTouchEnabled());

    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);

    // Push a VC whose content is not focusable. Removing the old (focused)
    // content bubbles focus up to the window; the fix must then move it onto
    // the nav bar's back button rather than leaving it stranded on the window.
    auto bareVC = std::make_shared<BareContentVC>(env.app);
    nav->pushViewController(bareVC);

    auto focused = env.window->getFocusedView().lock();
    auto expectedBackButton = navBarOf(nav)->firstFocusableDescendant();
    ASSERT_TRUE(expectedBackButton != nullptr);
    ASSERT_TRUE(focused.get() != env.window.get());  // not stranded on the window
    ASSERT_TRUE(focused == expectedBackButton);
}

TEST(navigation_push_vc_with_content_focuses_content) {
    // Regression guard: when the pushed content HAS a focusable view, focus
    // goes to it — not the back button. Preserves the 8d9e376 behavior.
    auto env = makeNavTestEnv();

    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);

    auto pushed = std::make_shared<ButtonContentVC>(env.app);
    nav->pushViewController(pushed);

    auto focused = env.window->getFocusedView().lock();
    ASSERT_TRUE(focused == pushed->button);
}

namespace {

struct TouchNavTestEnv {
    std::shared_ptr<NavTestWindow> window;
    std::shared_ptr<NavTestApplication> app;
};

static TouchNavTestEnv makeTouchNavTestEnv() {
    auto window = std::make_shared<NavTestWindow>(MakeSize(160, 128));
    window->setTouchEnabled();
    auto app = std::make_shared<NavTestApplication>(window);
    window->setApp(app);
    return {window, app};
}

}  // namespace

TEST(touch_push_while_latent_stays_latent) {
    auto env = makeTouchNavTestEnv();
    auto nav = NavigationViewController::create(env.app,
        std::make_shared<ButtonContentVC>(env.app));
    env.app->setRootViewController(nav);
    ASSERT_FALSE(env.window->isFocusEngaged());

    nav->pushViewController(std::make_shared<ButtonContentVC>(env.app));
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_push_while_engaged_focuses_new_content) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    rootVC->button->becomeFocused();

    auto pushed = std::make_shared<ButtonContentVC>(env.app);
    nav->pushViewController(pushed);
    ASSERT_TRUE(env.window->getFocusedView().lock() == pushed->button);
}

TEST(touch_pop_while_engaged_focuses_revealed_content) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    auto nav = NavigationViewController::create(env.app, rootVC);
    env.app->setRootViewController(nav);
    rootVC->button->becomeFocused();
    auto pushed = std::make_shared<ButtonContentVC>(env.app);
    nav->pushViewController(pushed);
    ASSERT_TRUE(env.window->isFocusEngaged());

    nav->popViewController();
    ASSERT_TRUE(env.window->getFocusedView().lock() == rootVC->button);
}

TEST(touch_pop_while_latent_stays_latent) {
    auto env = makeTouchNavTestEnv();
    auto nav = NavigationViewController::create(env.app,
        std::make_shared<ButtonContentVC>(env.app));
    env.app->setRootViewController(nav);
    nav->pushViewController(std::make_shared<ButtonContentVC>(env.app));

    nav->popViewController();
    ASSERT_FALSE(env.window->isFocusEngaged());
}

namespace {

// Minimal two-item collection VC for the latent-start pin.
class TwoItemCollectionVC : public CollectionViewController {
public:
    TwoItemCollectionVC(std::shared_ptr<Application> app) : CollectionViewController(app) {
        this->setItemSize(MakeSize(0, 24));
        this->setLayout(CollectionViewLayout::VerticalList);
    }
    size_t numberOfItems() const override { return 2; }
    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(size_t index, Rect frame) override {
        return std::make_shared<CollectionViewCell>(frame);
    }
};

}  // namespace

TEST(touch_collection_vc_starts_latent) {
    auto env = makeTouchNavTestEnv();
    env.app->setRootViewController(std::make_shared<TwoItemCollectionVC>(env.app));
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(dpad_collection_vc_still_autofocuses_first_cell) {
    auto env = makeNavTestEnv();
    env.app->setRootViewController(std::make_shared<TwoItemCollectionVC>(env.app));
    ASSERT_TRUE(env.window->isFocusEngaged());
}

TEST(touch_modal_present_while_engaged_enters_modal) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    env.app->setRootViewController(rootVC);
    rootVC->button->becomeFocused();

    auto modal = std::make_shared<ButtonContentVC>(env.app);
    env.app->presentViewController(modal);
    ASSERT_TRUE(env.window->getFocusedView().lock() == modal->button);
}

TEST(touch_modal_present_while_latent_stays_latent) {
    auto env = makeTouchNavTestEnv();
    env.app->setRootViewController(std::make_shared<ButtonContentVC>(env.app));

    env.app->presentViewController(std::make_shared<ButtonContentVC>(env.app));
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_modal_present_engaged_with_no_focusable_drops_to_latent) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    env.app->setRootViewController(rootVC);
    rootVC->button->becomeFocused();

    env.app->presentViewController(std::make_shared<BareContentVC>(env.app));
    // No live focus behind the dimmer.
    ASSERT_FALSE(env.window->isFocusEngaged());
}

TEST(touch_modal_dismiss_restores_focus_only_if_still_engaged) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    env.app->setRootViewController(rootVC);
    rootVC->button->becomeFocused();

    auto modal = std::make_shared<ButtonContentVC>(env.app);
    env.app->presentViewController(modal);           // engaged -> modal button
    env.app->dismissViewController();                // still engaged inside modal
    ASSERT_TRUE(env.window->getFocusedView().lock() == rootVC->button);
}

TEST(touch_modal_dismiss_stays_latent_if_user_went_latent_inside) {
    auto env = makeTouchNavTestEnv();
    auto rootVC = std::make_shared<ButtonContentVC>(env.app);
    env.app->setRootViewController(rootVC);
    rootVC->button->becomeFocused();

    auto modal = std::make_shared<ButtonContentVC>(env.app);
    env.app->presentViewController(modal);
    env.window->becomeFocused();                     // user went latent mid-modal
    env.app->dismissViewController();
    ASSERT_FALSE(env.window->isFocusEngaged());
}
