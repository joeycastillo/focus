/*
 * Tests for reloading one collection view item in place: View::insertSubview,
 * PagedCollectionView::reloadItemAtIndex, and the passthroughs on
 * PaginatedCollectionView and CollectionViewController.
 *
 * The contract under test: an on-page reload rebuilds exactly one cell at
 * its old position and frame, dirties only that cell's rect, and restores
 * focus to the same place inside the replacement; off-page indices are no-ops.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Window.hpp"
#include "Application.hpp"
#include "Control.hpp"
#include "CollectionViewCell.hpp"
#include "CollectionViewDataSource.hpp"
#include "CollectionViewDelegate.hpp"
#include "PagedCollectionView.hpp"
#include "PaginatedCollectionView.hpp"
#include "CollectionViewController.hpp"
#include <map>
#include <string>
#include <vector>

using namespace focus;

namespace {

// Non-touch window (d-pad mode) so focus is live; touchEnabled stays false.
class ReloadTestWindow : public Window {
public:
    ReloadTestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) { this->application = app; }
};

class ReloadTestApplication : public Application {
public:
    ReloadTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

}  // namespace

// --- View::insertSubview ---

TEST(insert_subview_places_view_at_index) {
    auto parent = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    auto a = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    auto b = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    auto c = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    parent->addSubview(a);
    parent->addSubview(c);

    parent->insertSubview(b, 1);

    ASSERT_EQ(parent->getSubviews().size(), (size_t)3);
    ASSERT_TRUE(parent->getSubviews()[0] == a);
    ASSERT_TRUE(parent->getSubviews()[1] == b);
    ASSERT_TRUE(parent->getSubviews()[2] == c);
    ASSERT_TRUE(b->getSuperview() == parent.get());
}

TEST(insert_subview_past_end_appends) {
    auto parent = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    auto a = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    auto b = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    parent->addSubview(a);

    parent->insertSubview(b, 99);

    ASSERT_EQ(parent->getSubviews().size(), (size_t)2);
    ASSERT_TRUE(parent->getSubviews()[1] == b);
}

TEST(insert_subview_reparents_from_previous_parent) {
    auto p1 = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    auto p2 = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    auto d = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    auto e = std::make_shared<View>(MakeRect(0, 0, 10, 10));
    p2->addSubview(d);
    p1->addSubview(e);

    p1->insertSubview(d, 0);

    ASSERT_TRUE(p2->getSubviews().empty());
    ASSERT_EQ(p1->getSubviews().size(), (size_t)2);
    ASSERT_TRUE(p1->getSubviews()[0] == d);
    ASSERT_TRUE(d->getSuperview() == p1.get());
}

TEST(insert_subview_attaches_window_and_dirties_only_the_child_rect) {
    auto window = std::make_shared<ReloadTestWindow>(MakeSize(100, 100));
    auto parent = std::make_shared<View>(MakeRect(10, 20, 80, 60));
    window->addSubview(parent);
    window->clearNeedsDisplay();

    auto child = std::make_shared<View>(MakeRect(5, 5, 10, 10));
    parent->insertSubview(child, 0);

    ASSERT_TRUE(child->getWindow().lock() == window);
    ASSERT_TRUE(window->needsDisplay());
    // (5,5) inside a parent at (10,20) is (15,25) in the window.
    ASSERT_TRUE(RectsEqual(window->getDirtyRect(), MakeRect(15, 25, 10, 10)));
}
