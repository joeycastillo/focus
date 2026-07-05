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
