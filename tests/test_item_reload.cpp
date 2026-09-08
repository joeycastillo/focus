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

// --- PagedCollectionView::reloadItemAtIndex ---

namespace {

// Data source over a list of names. Records every cell request and the cell
// it handed out; can run variable-size and can host controls inside cells.
class RecordingDataSource : public CollectionViewDataSource {
public:
    std::vector<std::string> names;
    std::map<size_t, int> requests;
    std::map<size_t, std::shared_ptr<CollectionViewCell>> cells;
    std::map<size_t, int> heights;   // non-empty switches the list to variable-size mode
    int controlsPerCell = 0;

    size_t numberOfItems(const CollectionView*) const override {
        return this->names.size();
    }
    Size sizeForItemAtIndex(const CollectionView*, size_t index) const override {
        auto it = this->heights.find(index);
        if (it == this->heights.end()) return {0, 0};
        return MakeSize(0, it->second);
    }
    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(CollectionView*, size_t index, Rect frame) override {
        this->requests[index]++;
        auto cell = std::make_shared<CollectionViewCell>(frame);
        for (int i = 0; i < this->controlsPerCell; i++) {
            cell->addSubview(std::make_shared<Control>(MakeRect(i * 10, 0, 10, frame.size.height)));
        }
        this->cells[index] = cell;
        return cell;
    }
};

// Records which item indices gained and lost focus, in order.
class RecordingDelegate : public CollectionViewDelegate {
public:
    std::vector<size_t> focused;
    std::vector<size_t> unfocused;
    void didFocusItemAtIndex(CollectionView*, size_t index, CollectionViewCell&) override {
        this->focused.push_back(index);
    }
    void didUnfocusItemAtIndex(CollectionView*, size_t index, CollectionViewCell&) override {
        this->unfocused.push_back(index);
    }
};

// A vertical list of 20px rows in an 80x60 frame at window origin (10,20):
// three rows per page. Populated and attached; nothing focused yet.
std::shared_ptr<PagedCollectionView> makeList(std::shared_ptr<ReloadTestWindow>& window,
                                              RecordingDataSource& ds,
                                              RecordingDelegate* delegate,
                                              size_t count) {
    window = std::make_shared<ReloadTestWindow>(MakeSize(100, 100));
    for (size_t i = 0; i < count; i++) ds.names.push_back("item " + std::to_string(i));
    auto cv = std::make_shared<PagedCollectionView>(MakeRect(10, 20, 80, 60));
    cv->setLayout(CollectionViewLayout::VerticalList);
    cv->setItemSize(MakeSize(0, 20));
    cv->setDataSource(&ds);
    if (delegate) cv->setDelegate(delegate);
    window->addSubview(cv);
    cv->reloadData();
    return cv;
}

}  // namespace

TEST(reload_on_page_rebuilds_one_cell_in_place) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    auto cv = makeList(window, ds, nullptr, 7);
    auto before = cv->getSubviews();
    ASSERT_EQ(before.size(), (size_t)3);
    ds.requests.clear();

    cv->reloadItemAtIndex(1);

    ASSERT_EQ(ds.requests.size(), (size_t)1);
    ASSERT_EQ(ds.requests[1], 1);
    auto after = cv->getSubviews();
    ASSERT_EQ(after.size(), (size_t)3);
    ASSERT_TRUE(after[0] == before[0]);
    ASSERT_TRUE(after[1] != before[1]);
    ASSERT_TRUE(after[1] == ds.cells[1]);
    ASSERT_TRUE(after[2] == before[2]);
    ASSERT_TRUE(RectsEqual(after[1]->getFrame(), before[1]->getFrame()));
    ASSERT_TRUE(before[1]->getSuperview() == nullptr);
}

TEST(reload_uses_the_current_page) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    auto cv = makeList(window, ds, nullptr, 7);
    cv->goToPage(1);
    ds.requests.clear();

    cv->reloadItemAtIndex(4);
    cv->reloadItemAtIndex(1);   // on page 0, not the current page

    ASSERT_EQ(ds.requests.size(), (size_t)1);
    ASSERT_EQ(ds.requests[4], 1);
    ASSERT_TRUE(cv->getSubviews()[1] == ds.cells[4]);
}

TEST(reload_off_page_and_invalid_indices_are_no_ops) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    auto cv = makeList(window, ds, nullptr, 7);
    auto before = cv->getSubviews();
    ds.requests.clear();
    window->clearNeedsDisplay();

    cv->reloadItemAtIndex(3);      // page 1
    cv->reloadItemAtIndex(6);      // page 2
    cv->reloadItemAtIndex(7);      // past the end
    cv->reloadItemAtIndex(1000);

    ASSERT_TRUE(ds.requests.empty());
    ASSERT_TRUE(cv->getSubviews() == before);
    ASSERT_FALSE(window->needsDisplay());

    // No data source, and data source set but reloadData() never called.
    auto bare = std::make_shared<PagedCollectionView>(MakeRect(0, 0, 80, 60));
    bare->setItemSize(MakeSize(0, 20));
    bare->reloadItemAtIndex(0);
    RecordingDataSource ds2;
    ds2.names = {"a"};
    bare->setDataSource(&ds2);
    bare->reloadItemAtIndex(0);
    ASSERT_TRUE(ds2.requests.empty());
    ASSERT_TRUE(bare->getSubviews().empty());
}

TEST(reload_dirties_only_the_cell_rect) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    auto cv = makeList(window, ds, nullptr, 7);
    window->clearNeedsDisplay();

    cv->reloadItemAtIndex(1);

    ASSERT_TRUE(window->needsDisplay());
    // Row 1 is (0,20) in the list, which sits at (10,20) in the window.
    ASSERT_TRUE(RectsEqual(window->getDirtyRect(), MakeRect(10, 40, 80, 20)));
}

TEST(reload_restores_focus_to_the_replacement_cell) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    RecordingDelegate delegate;
    auto cv = makeList(window, ds, &delegate, 7);
    cv->getSubviews()[1]->becomeFocused();
    delegate.focused.clear();
    delegate.unfocused.clear();
    window->clearNeedsDisplay();

    cv->reloadItemAtIndex(1);

    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[1]);
    ASSERT_EQ(delegate.unfocused.size(), (size_t)1);
    ASSERT_EQ(delegate.unfocused[0], (size_t)1);
    ASSERT_EQ(delegate.focused.size(), (size_t)1);
    ASSERT_EQ(delegate.focused[0], (size_t)1);
    // The focus hop through the window adds nothing to the dirty rect.
    ASSERT_TRUE(RectsEqual(window->getDirtyRect(), MakeRect(10, 40, 80, 20)));
}

TEST(reload_restores_focus_to_the_same_descendant) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    ds.controlsPerCell = 3;
    auto cv = makeList(window, ds, nullptr, 7);
    auto oldCell = cv->getSubviews()[1];
    oldCell->getSubviews()[1]->becomeFocused();   // the second control in the row

    cv->reloadItemAtIndex(1);

    auto newCell = ds.cells[1];
    ASSERT_TRUE(cv->getSubviews()[1] == newCell);
    ASSERT_TRUE(window->getFocusedView().lock() == newCell->getSubviews()[1]);
}

TEST(reload_leaves_focus_alone_elsewhere) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    RecordingDelegate delegate;
    auto cv = makeList(window, ds, &delegate, 7);
    auto cell0 = cv->getSubviews()[0];
    cell0->becomeFocused();
    delegate.focused.clear();
    delegate.unfocused.clear();

    cv->reloadItemAtIndex(1);

    ASSERT_TRUE(window->getFocusedView().lock() == cell0);
    ASSERT_TRUE(delegate.focused.empty());
    ASSERT_TRUE(delegate.unfocused.empty());
}

TEST(reload_keeps_the_old_frame_in_variable_size_mode) {
    std::shared_ptr<ReloadTestWindow> window;
    RecordingDataSource ds;
    ds.heights = {{0, 20}, {1, 30}, {2, 10}, {3, 20}};   // 20+30+10 fills page 0 exactly
    auto cv = makeList(window, ds, nullptr, 4);
    ASSERT_EQ(cv->getSubviews().size(), (size_t)3);
    Rect frame1 = cv->getSubviews()[1]->getFrame();
    ASSERT_TRUE(RectsEqual(frame1, MakeRect(0, 20, 80, 30)));

    // Contract violation: the size changed. Expect one warning line on stderr.
    ds.heights[1] = 40;
    cv->reloadItemAtIndex(1);

    ASSERT_TRUE(cv->getSubviews()[1] == ds.cells[1]);
    ASSERT_TRUE(RectsEqual(ds.cells[1]->getFrame(), frame1));
    ASSERT_TRUE(RectsEqual(cv->getSubviews()[2]->getFrame(), MakeRect(0, 50, 80, 10)));
}

TEST(reload_keeps_grid_navigation_order) {
    auto window = std::make_shared<ReloadTestWindow>(MakeSize(100, 100));
    RecordingDataSource ds;
    for (size_t i = 0; i < 6; i++) ds.names.push_back("item " + std::to_string(i));
    // 90x60 grid of 30x30 cells: three columns, two rows, six per page.
    auto cv = std::make_shared<PagedCollectionView>(MakeRect(0, 0, 90, 60));
    cv->setLayout(CollectionViewLayout::Grid);
    cv->setItemSize(MakeSize(30, 30));
    cv->setDataSource(&ds);
    window->addSubview(cv);
    cv->reloadData();
    cv->getSubviews()[1]->becomeFocused();

    cv->reloadItemAtIndex(1);
    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[1]);

    // If the replacement had been appended instead of reinserted, RIGHT from
    // it would hit the row edge and DOWN would run off the page.
    Event right = {FOCUS_EVENT_DIRECTION_RIGHT, 0, 0};
    ASSERT_TRUE(cv->handleEvent(right));
    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[2]);
    Event down = {FOCUS_EVENT_DIRECTION_DOWN, 0, 0};
    ASSERT_TRUE(cv->handleEvent(down));
    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[5]);
    Event left = {FOCUS_EVENT_DIRECTION_LEFT, 0, 0};
    ASSERT_TRUE(cv->handleEvent(left));
    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[4]);
    Event up = {FOCUS_EVENT_DIRECTION_UP, 0, 0};
    ASSERT_TRUE(cv->handleEvent(up));
    ASSERT_TRUE(window->getFocusedView().lock() == ds.cells[1]);
}

// --- Passthroughs ---

TEST(paginated_collection_view_forwards_reload_item) {
    auto window = std::make_shared<ReloadTestWindow>(MakeSize(100, 100));
    RecordingDataSource ds;
    for (size_t i = 0; i < 7; i++) ds.names.push_back("item " + std::to_string(i));
    auto pv = std::make_shared<PaginatedCollectionView>(MakeRect(10, 0, 80, 100));
    pv->setLayout(CollectionViewLayout::VerticalList);
    pv->setItemSize(MakeSize(0, 20));
    pv->setPaginationStyle(PaginationStyle::Arrows);
    pv->setDataSource(&ds);
    window->addSubview(pv);
    pv->reloadData();
    auto chrome = pv->getSubviews();   // the collection view plus its arrow indicators
    ds.requests.clear();

    pv->reloadItemAtIndex(1);

    ASSERT_EQ(ds.requests.size(), (size_t)1);
    ASSERT_EQ(ds.requests[1], 1);
    ASSERT_TRUE(pv->getSubviews() == chrome);
}

namespace {

// Five-row collection controller that counts cell requests per index.
class ReloadCountingVC : public CollectionViewController {
public:
    ReloadCountingVC(std::shared_ptr<Application> app) : CollectionViewController(app) {
        this->setItemSize(MakeSize(0, 20));
        this->setLayout(CollectionViewLayout::VerticalList);
    }
    std::map<size_t, int> requests;
    size_t numberOfItems() const override { return 5; }
    std::shared_ptr<CollectionViewCell> cellForItemAtIndex(size_t index, Rect frame) override {
        this->requests[index]++;
        return std::make_shared<CollectionViewCell>(frame);
    }
};

}  // namespace

TEST(collection_view_controller_forwards_reload_item) {
    auto window = std::make_shared<ReloadTestWindow>(MakeSize(100, 100));
    auto app = std::make_shared<ReloadTestApplication>(window);
    window->setApp(app);

    // Before the view exists there is nothing to reload, and no crash.
    auto detached = std::make_shared<ReloadCountingVC>(app);
    detached->reloadItemAtIndex(0);
    ASSERT_TRUE(detached->requests.empty());

    auto vc = std::make_shared<ReloadCountingVC>(app);
    app->setRootViewController(vc);
    ASSERT_FALSE(vc->requests.empty());
    vc->requests.clear();

    vc->reloadItemAtIndex(1);

    ASSERT_EQ(vc->requests.size(), (size_t)1);
    ASSERT_EQ(vc->requests[1], 1);
}
