/*
 * Tests for appearance callbacks when opaque modals cover and uncover
 * view controllers, when the root is replaced, and when hidden
 * containers change.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"
#include "Application.hpp"
#include "AlertViewController.hpp"
#include "ViewController.hpp"
#include "Window.hpp"
#include "NavigationViewController.hpp"
#include "TabViewController.hpp"
#include "CollectionViewController.hpp"
#include "CollectionViewCell.hpp"
#include <algorithm>
#include <functional>

using namespace focus;

namespace {

class AppearanceTestWindow : public Window {
public:
    AppearanceTestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) { this->application = app; }
};

class AppearanceTestApplication : public Application {
public:
    AppearanceTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

using EventLog = std::vector<std::string>;

// Records each lifecycle call as "<name>.<event>" into a shared log.
class RecordingVC : public ViewController {
public:
    RecordingVC(std::shared_ptr<Application> app, std::string name,
                std::shared_ptr<EventLog> log, bool opaque = true)
        : ViewController(app), name(std::move(name)), log(std::move(log)), opaque(opaque) {}

    void viewWillAppear() override {
        ViewController::viewWillAppear();
        this->record("willAppear");
    }
    void viewDidLayoutSubviews() override { this->record("didLayout"); }
    void viewDidAppear() override { this->record("didAppear"); }
    void viewWillDisappear() override {
        this->record("willDisappear");
        if (this->onWillDisappear) this->onWillDisappear();
    }
    void viewDidDisappear() override {
        this->record("didDisappear");
        ViewController::viewDidDisappear();
    }

    int createCount = 0;
    std::shared_ptr<Button> button;
    std::function<void()> onWillDisappear;

protected:
    void createView() override {
        this->createCount++;
        this->view = std::make_shared<View>(MakeRect(0, 0, 160, 128));
        this->view->setOpaque(this->opaque);
        this->button = std::make_shared<Button>(MakeRect(0, 0, 160, 20), "Go");
        this->view->addSubview(this->button);
        this->record("createView");
    }
    void destroyView() override {
        this->record("destroyView");
        ViewController::destroyView();
    }
    void record(const std::string& event) { this->log->push_back(this->name + "." + event); }

    std::string name;
    std::shared_ptr<EventLog> log;
    bool opaque;
};

class TwoItemCollectionVC : public CollectionViewController {
public:
    TwoItemCollectionVC(std::shared_ptr<Application> app) : CollectionViewController(app) {
        this->setItemSize(MakeSize(0, 24));
        this->setLayout(CollectionViewLayout::VerticalList);
    }
    size_t numberOfItems() const override { return 2; }
    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(size_t, Rect frame) override {
        return std::make_shared<CollectionViewCell>(frame);
    }
    std::shared_ptr<PaginatedCollectionView> paginated() const { return this->getPaginatedView(); }
};

// A view controller with nothing focusable.
class BareVC : public ViewController {
public:
    BareVC(std::shared_ptr<Application> app) : ViewController(app) {}

protected:
    void createView() override { this->view = std::make_shared<View>(MakeRect(0, 0, 160, 128)); }
};

struct AppearanceTestEnv {
    std::shared_ptr<AppearanceTestWindow> window;
    std::shared_ptr<AppearanceTestApplication> app;
    std::shared_ptr<EventLog> log;

    std::shared_ptr<RecordingVC> make(const std::string& name, bool opaque = true) {
        return std::make_shared<RecordingVC>(this->app, name, this->log, opaque);
    }
};

static AppearanceTestEnv makeEnv() {
    auto window = std::make_shared<AppearanceTestWindow>(MakeSize(160, 128));
    auto app = std::make_shared<AppearanceTestApplication>(window);
    window->setApp(app);
    return {window, app, std::make_shared<EventLog>()};
}

static std::string joined(const EventLog& log) {
    std::string out;
    for (const auto& event : log) {
        if (!out.empty()) out += " ";
        out += event;
    }
    return out;
}

static bool logged(const EventLog& log, const std::string& event) {
    return std::find(log.begin(), log.end(), event) != log.end();
}

// Position of a view among the window's subviews, or -1.
static int windowIndex(const AppearanceTestEnv& env, const std::shared_ptr<View>& view) {
    const auto& subviews = env.window->getSubviews();
    auto it = std::find(subviews.begin(), subviews.end(), view);
    return it == subviews.end() ? -1 : (int)(it - subviews.begin());
}

static std::shared_ptr<View> findById(const std::shared_ptr<View>& view, const std::string& id) {
    if (view->accessibilityIdentifier == id) return view;
    for (const auto& child : view->getSubviews()) {
        if (auto found = findById(child, id)) return found;
    }
    return nullptr;
}

}  // namespace

// --- Covering and uncovering ---

TEST(opaque_modal_hides_the_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    env.app->setRootViewController(a);
    env.log->clear();

    env.app->presentViewController(b);

    ASSERT_STREQ(joined(*env.log),
        "b.createView b.willAppear b.didLayout "
        "a.willDisappear a.didDisappear a.destroyView "
        "b.didAppear");
    ASSERT_TRUE(a->getView() == nullptr);
    ASSERT_EQ((int)env.window->getSubviews().size(), 1);
}

TEST(dismissing_opaque_modal_rebuilds_and_shows_the_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.log->clear();

    env.app->dismissViewController();

    ASSERT_STREQ(joined(*env.log),
        "b.willDisappear b.didDisappear b.destroyView "
        "a.createView a.willAppear a.didLayout a.didAppear");
    ASSERT_EQ(a->createCount, 2);
    ASSERT_EQ(windowIndex(env, a->getView()), 0);
    ASSERT_EQ((int)env.window->getSubviews().size(), 1);
}

TEST(translucent_modal_leaves_the_root_alone) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b", false);
    env.app->setRootViewController(a);
    env.log->clear();

    env.app->presentViewController(b);
    env.app->dismissViewController();

    ASSERT_FALSE(logged(*env.log, "a.willDisappear"));
    ASSERT_FALSE(logged(*env.log, "a.willAppear"));
    ASSERT_EQ(a->createCount, 1);
}

TEST(opaque_modal_over_translucent_modal_hides_both) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b", false);
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.log->clear();

    env.app->presentViewController(c);

    ASSERT_STREQ(joined(*env.log),
        "c.createView c.willAppear c.didLayout "
        "b.willDisappear b.didDisappear b.destroyView "
        "a.willDisappear a.didDisappear a.destroyView "
        "c.didAppear");
    // Everything under c, including b's dimmer, is hidden.
    for (const auto& subview : env.window->getSubviews()) {
        if (subview != c->getView()) ASSERT_TRUE(subview->isHidden());
    }
}

TEST(dismissing_opaque_modal_restores_translucent_stack_in_order) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b", false);
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.app->presentViewController(c);
    env.log->clear();

    env.app->dismissViewController();

    ASSERT_TRUE(logged(*env.log, "a.didAppear"));
    ASSERT_TRUE(logged(*env.log, "b.didAppear"));
    // Root, then b's dimmer, then b.
    ASSERT_EQ((int)env.window->getSubviews().size(), 3);
    ASSERT_EQ(windowIndex(env, a->getView()), 0);
    ASSERT_EQ(windowIndex(env, b->getView()), 2);
    ASSERT_FALSE(env.window->getSubviews()[1]->isHidden());
}

TEST(uncovered_modal_keeps_its_focus_trap) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b", false);
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.app->presentViewController(c);

    env.app->dismissViewController();

    ASSERT_TRUE(b->getView() != nullptr);
    ASSERT_TRUE(b->getView()->getClipsFocus());
}

TEST(opaque_modal_over_opaque_modal_hides_only_the_one_below) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.log->clear();

    env.app->presentViewController(c);
    ASSERT_FALSE(logged(*env.log, "a.willDisappear"));
    ASSERT_TRUE(logged(*env.log, "b.didDisappear"));

    env.log->clear();
    env.app->dismissViewController();
    ASSERT_FALSE(logged(*env.log, "a.willAppear"));
    ASSERT_TRUE(logged(*env.log, "b.didAppear"));
}

TEST(opaque_modal_hides_the_navigation_top) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto nav = NavigationViewController::create(env.app, a);
    env.app->setRootViewController(nav);
    env.log->clear();

    env.app->presentViewController(env.make("m"));
    ASSERT_TRUE(logged(*env.log, "a.didDisappear"));
    ASSERT_TRUE(nav->getView() == nullptr);

    env.app->dismissViewController();
    ASSERT_TRUE(logged(*env.log, "a.didAppear"));
    ASSERT_EQ(a->createCount, 2);
    ASSERT_TRUE(nav->getView() != nullptr);
}

// --- Focus ---

TEST(dismissing_opaque_modal_focuses_the_rebuilt_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    env.app->setRootViewController(a);
    auto oldButton = a->button;
    ASSERT_TRUE(env.window->getFocusedView().lock() == oldButton);

    env.app->presentViewController(env.make("b"));
    env.app->dismissViewController();

    auto focused = env.window->getFocusedView().lock();
    ASSERT_TRUE(focused == a->button);
    ASSERT_TRUE(focused != oldButton);
}

TEST(dismissing_translucent_modal_restores_the_same_view) {
    auto env = makeEnv();
    auto a = env.make("a");
    env.app->setRootViewController(a);
    auto button = a->button;

    env.app->presentViewController(env.make("b", false));
    env.app->dismissViewController();

    ASSERT_TRUE(env.window->getFocusedView().lock() == button);
}

// --- Presenting from inside callbacks ---

TEST(opaque_modal_presented_during_dismissal_keeps_the_root_hidden) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    b->onWillDisappear = [&]() { env.app->presentViewController(c); };
    env.log->clear();

    env.app->dismissViewController();

    ASSERT_FALSE(logged(*env.log, "a.willAppear"));
    ASSERT_TRUE(a->getView() == nullptr);
    ASSERT_TRUE(env.app->activeViewController() == c);
}

TEST(translucent_modal_presented_during_dismissal_reveals_the_root_beneath) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c", false);
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    b->onWillDisappear = [&]() { env.app->presentViewController(c); };

    env.app->dismissViewController();

    ASSERT_TRUE(env.app->activeViewController() == c);
    ASSERT_EQ(windowIndex(env, a->getView()), 0);
    ASSERT_TRUE(windowIndex(env, c->getView()) > windowIndex(env, a->getView()));
}

TEST(presenting_while_covering_skips_a_modal_covered_meanwhile) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto m = env.make("m", false);
    auto n = env.make("n");
    auto x = env.make("x");
    env.app->setRootViewController(a);
    env.app->presentViewController(m);
    m->onWillDisappear = [&]() { env.app->presentViewController(x); };

    env.app->presentViewController(n);

    ASSERT_TRUE(env.app->activeViewController() == x);
    ASSERT_FALSE(logged(*env.log, "n.didAppear"));
    ASSERT_TRUE(env.window->getFocusedView().lock() == x->button);
}

TEST(translucent_modal_presented_while_covering_keeps_focus_on_top) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto m = env.make("m");
    auto x = env.make("x", false);
    env.app->setRootViewController(a);
    bool presented = false;
    a->onWillDisappear = [&]() {
        if (presented) return;
        presented = true;
        env.app->presentViewController(x);
    };

    env.app->presentViewController(m);

    ASSERT_TRUE(env.app->activeViewController() == x);
    ASSERT_TRUE(logged(*env.log, "m.didAppear"));
    ASSERT_TRUE(env.window->getFocusedView().lock() == x->button);
}

// --- Replacing the root, dismissing all ---

TEST(set_root_removes_modals_without_revealing_the_old_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.log->clear();

    env.app->setRootViewController(c);

    ASSERT_STREQ(joined(*env.log),
        "b.willDisappear b.didDisappear b.destroyView "
        "c.createView c.willAppear c.didLayout c.didAppear");
    ASSERT_FALSE(env.app->isModalPresented());
    ASSERT_EQ((int)env.window->getSubviews().size(), 1);
}

TEST(set_root_removes_translucent_modals_then_the_old_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b", false);
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.log->clear();

    env.app->setRootViewController(c);

    ASSERT_STREQ(joined(*env.log),
        "b.willDisappear b.didDisappear b.destroyView "
        "a.willDisappear a.didDisappear a.destroyView "
        "c.createView c.willAppear c.didLayout c.didAppear");
    ASSERT_EQ((int)env.window->getSubviews().size(), 1);
}

TEST(set_root_skips_alert_completions) {
    auto env = makeEnv();
    env.app->setRootViewController(env.make("a"));
    bool completed = false;
    env.app->presentViewController(AlertViewController::create(
        env.app, "Title", "Message", {"OK"}, [&](int) { completed = true; }));

    env.app->setRootViewController(env.make("c"));

    ASSERT_FALSE(completed);
    ASSERT_FALSE(env.app->isModalPresented());
}

TEST(modal_presented_during_root_teardown_stays_above_the_new_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    auto d = env.make("d", false);
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    b->onWillDisappear = [&]() { env.app->presentViewController(d); };

    env.app->setRootViewController(c);

    ASSERT_TRUE(env.app->activeViewController() == d);
    ASSERT_EQ(windowIndex(env, c->getView()), 0);
    ASSERT_TRUE(windowIndex(env, d->getView()) > windowIndex(env, c->getView()));
}

TEST(dismiss_all_reveals_only_the_root) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    env.app->presentViewController(c);
    env.log->clear();

    env.app->dismissAllViewControllers();

    ASSERT_STREQ(joined(*env.log),
        "c.willDisappear c.didDisappear c.destroyView "
        "a.createView a.willAppear a.didLayout a.didAppear");
    ASSERT_EQ(b->createCount, 1);
    ASSERT_FALSE(env.app->isModalPresented());
}

TEST(set_root_called_during_root_teardown_is_replaced_cleanly) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto c = env.make("c");
    auto d = env.make("d");
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    bool fired = false;
    b->onWillDisappear = [&]() {
        if (!fired) {
            fired = true;
            env.app->setRootViewController(d);
        }
    };
    env.log->clear();

    env.app->setRootViewController(c);

    ASSERT_EQ((int)env.window->getSubviews().size(), 1);
    ASSERT_EQ(windowIndex(env, c->getView()), 0);
    ASSERT_TRUE(d->getView() == nullptr);
    ASSERT_TRUE(logged(*env.log, "d.didDisappear"));
    ASSERT_FALSE(logged(*env.log, "a.willDisappear"));
    ASSERT_TRUE(env.app->activeViewController() == c);
}

TEST(modal_presented_during_dismiss_all_stays_up) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto d = env.make("d", false);
    env.app->setRootViewController(a);
    env.app->presentViewController(b);
    b->onWillDisappear = [&]() { env.app->presentViewController(d); };

    env.app->dismissAllViewControllers();

    ASSERT_TRUE(env.app->activeViewController() == d);
    ASSERT_TRUE(a->getView() != nullptr);
}

// --- Tab focus ---

TEST(popping_back_to_a_tab_controller_keeps_its_selected_tab) {
    auto env = makeEnv();
    auto tabs = TabViewController::create(env.app);
    tabs->addTab("A", env.make("a"));
    tabs->addTab("B", env.make("b"));
    auto nav = NavigationViewController::create(env.app, tabs);
    env.app->setRootViewController(nav);
    tabs->selectTab(1);

    nav->pushViewController(env.make("c"));
    nav->popViewController();

    ASSERT_EQ((int)tabs->getSelectedTab(), 1);
    auto item = findById(tabs->getView(), "tab-item-1");
    ASSERT_TRUE(env.window->getFocusedView().lock() == item);
}

TEST(entering_tabs_from_the_end_lands_on_the_selected_tab_when_content_has_nothing) {
    auto env = makeEnv();
    auto tabs = TabViewController::create(env.app);
    tabs->addTab("A", std::make_shared<BareVC>(env.app));
    tabs->addTab("B", std::make_shared<BareVC>(env.app));
    tabs->addTab("C", std::make_shared<BareVC>(env.app));
    env.app->setRootViewController(tabs);
    tabs->selectTab(1);

    auto last = tabs->getView()->lastFocusableDescendant();
    ASSERT_TRUE(last == findById(tabs->getView(), "tab-item-1"));
}

TEST(entering_tabs_from_the_end_lands_on_content_when_it_has_some) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto tabs = TabViewController::create(env.app);
    tabs->addTab("A", a);
    tabs->addTab("B", b);
    env.app->setRootViewController(tabs);
    tabs->selectTab(1);

    ASSERT_TRUE(tabs->getView()->lastFocusableDescendant() == b->button);
}

// --- Hidden containers ---

TEST(push_onto_a_covered_navigation_controller_shows_after_uncover) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto nav = NavigationViewController::create(env.app, a);
    env.app->setRootViewController(nav);
    env.app->presentViewController(env.make("m"));
    env.log->clear();

    nav->pushViewController(b);
    ASSERT_EQ((int)nav->stackDepth(), 2);
    ASSERT_EQ(b->createCount, 0);

    env.app->dismissViewController();
    ASSERT_TRUE(logged(*env.log, "b.didAppear"));
    ASSERT_EQ(a->createCount, 1);
    ASSERT_TRUE(nav->topViewController() == b);
}

TEST(pop_on_a_covered_navigation_controller_shows_after_uncover) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto nav = NavigationViewController::create(env.app, a);
    env.app->setRootViewController(nav);
    nav->pushViewController(b);
    env.app->presentViewController(env.make("m"));
    env.log->clear();

    nav->popViewController();
    ASSERT_EQ((int)nav->stackDepth(), 1);
    ASSERT_TRUE(b->getNavigationController() == nullptr);
    ASSERT_TRUE(env.log->empty());

    env.app->dismissViewController();
    ASSERT_TRUE(logged(*env.log, "a.didAppear"));
    ASSERT_EQ(a->createCount, 2);
}

TEST(push_before_first_appearance_shows_the_pushed_controller) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto nav = NavigationViewController::create(env.app, a);

    nav->pushViewController(b);
    env.app->setRootViewController(nav);

    ASSERT_TRUE(nav->topViewController() == b);
    ASSERT_EQ(a->createCount, 0);
    ASSERT_EQ(b->createCount, 1);
}

TEST(select_tab_on_a_covered_tab_controller_shows_after_uncover) {
    auto env = makeEnv();
    auto a = env.make("a");
    auto b = env.make("b");
    auto tabs = TabViewController::create(env.app);
    tabs->addTab("A", a);
    tabs->addTab("B", b);
    env.app->setRootViewController(tabs);
    env.app->presentViewController(env.make("m"));
    env.log->clear();

    tabs->selectTab(1);
    ASSERT_EQ((int)tabs->getSelectedTab(), 1);
    ASSERT_EQ(b->createCount, 0);

    env.app->dismissViewController();
    ASSERT_TRUE(logged(*env.log, "b.didAppear"));
    ASSERT_EQ(a->createCount, 1);
}
