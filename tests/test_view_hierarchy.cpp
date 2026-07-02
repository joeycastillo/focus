/*
 * Tests for View hierarchy: hit testing, accessibility rect,
 * findAccessibilityElement, and convertPointFromWindow.
 */

#include "test_harness.hpp"
#include "View.hpp"
#include "Button.hpp"

using namespace focus;

// --- Hit testing (getViewForTouch) ---

TEST(hit_test_returns_deepest_subview) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    auto grandchild = std::make_shared<View>(MakeRect(5, 5, 50, 50));
    root->addSubview(child);
    child->addSubview(grandchild);

    // Touch at (20, 20) should hit grandchild (its frame is (5,5,50,50) inside child)
    // In root coords: child starts at (10,10), grandchild at (15,15)
    auto hit = root->getViewForTouch(MakePoint(20, 20)).lock();
    ASSERT_TRUE(hit != nullptr);
    ASSERT_TRUE(hit == grandchild);
}

TEST(hit_test_returns_front_view_on_overlap) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto back = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    auto front = std::make_shared<View>(MakeRect(50, 50, 100, 100));
    root->addSubview(back);
    root->addSubview(front);  // front is last = highest z-order

    // Touch at (60, 60) is in both back and front — should return front
    auto hit = root->getViewForTouch(MakePoint(60, 60)).lock();
    ASSERT_TRUE(hit != nullptr);
    ASSERT_TRUE(hit == front);
}

TEST(hit_test_returns_parent_when_outside_children) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    root->addSubview(child);

    // Touch at (200, 200) is inside root but outside child
    auto hit = root->getViewForTouch(MakePoint(200, 200)).lock();
    ASSERT_TRUE(hit != nullptr);
    ASSERT_TRUE(hit == root);
}

TEST(hit_test_skips_hidden_children) {
    // Hidden views (and their subtrees) should be invisible to hit-testing.
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 10, 100, 100));
    child->setHidden(true);
    root->addSubview(child);

    // Touch at (20, 20) should return root, not the hidden child
    auto hit = root->getViewForTouch(MakePoint(20, 20)).lock();
    ASSERT_TRUE(hit == root);
}

TEST(hit_test_with_bounds_offset) {
    // Simulate a scrolled container: bounds.origin = (0, 50) means
    // the container is scrolled down by 50 pixels.
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto scroller = std::make_shared<View>(MakeRect(0, 0, 480, 400));
    auto content = std::make_shared<View>(MakeRect(0, 0, 480, 100));
    root->addSubview(scroller);
    scroller->addSubview(content);

    // Scroll the scroller down by 50px
    scroller->setBounds(MakeRect(0, 50, 480, 400));

    // Content is at (0,0) in scroller coords, but scroller is scrolled.
    // Touch at (100, 0) — in scroller local coords with bounds offset,
    // this maps to local y = 0 + 50 = 50, which is inside content (y 0..100).
    auto hit = root->getViewForTouch(MakePoint(100, 0)).lock();
    ASSERT_TRUE(hit == content);
}

// --- accessibilityRect ---

TEST(accessibility_rect_top_level) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 20, 100, 50));
    root->addSubview(child);

    Rect rect = child->accessibilityRect();
    ASSERT_EQ(rect.origin.x, 10);
    ASSERT_EQ(rect.origin.y, 20);
    ASSERT_EQ(rect.size.width, 100);
    ASSERT_EQ(rect.size.height, 50);
}

TEST(accessibility_rect_nested) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto parent = std::make_shared<View>(MakeRect(10, 20, 200, 200));
    auto child = std::make_shared<View>(MakeRect(5, 5, 50, 50));
    root->addSubview(parent);
    parent->addSubview(child);

    Rect rect = child->accessibilityRect();
    // child's position in window coords = parent origin + child origin = (15, 25)
    ASSERT_EQ(rect.origin.x, 15);
    ASSERT_EQ(rect.origin.y, 25);
}

TEST(accessibility_rect_with_scroll_offset) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto scroller = std::make_shared<View>(MakeRect(0, 0, 480, 400));
    auto child = std::make_shared<View>(MakeRect(10, 100, 50, 50));
    root->addSubview(scroller);
    scroller->addSubview(child);

    // Scroll down by 30px
    scroller->setBounds(MakeRect(0, 30, 480, 400));

    Rect rect = child->accessibilityRect();
    // child frame is (10, 100) in scroller coords
    // scroller bounds origin is (0, 30), so offset = frame.origin - bounds.origin
    // In window: (10, 100 - 30) = (10, 70)
    ASSERT_EQ(rect.origin.x, 10);
    ASSERT_EQ(rect.origin.y, 70);
}

// --- findAccessibilityElement ---

TEST(find_accessibility_element_direct_child) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    child->accessibilityIdentifier = "target";
    root->addSubview(child);

    auto found = findAccessibilityElement(root, "target");
    ASSERT_TRUE(found != nullptr);
    ASSERT_TRUE(found == child);
}

TEST(find_accessibility_element_nested) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto container = std::make_shared<View>(MakeRect(0, 0, 200, 200));
    auto target = std::make_shared<View>(MakeRect(0, 0, 50, 50));
    target->accessibilityIdentifier = "deep-target";
    root->addSubview(container);
    container->addSubview(target);

    auto found = findAccessibilityElement(root, "deep-target");
    ASSERT_TRUE(found != nullptr);
    ASSERT_TRUE(found == target);
}

TEST(find_accessibility_element_not_found) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto found = findAccessibilityElement(root, "nonexistent");
    ASSERT_TRUE(found == nullptr);
}

TEST(find_accessibility_element_null_root) {
    auto found = findAccessibilityElement(nullptr, "anything");
    ASSERT_TRUE(found == nullptr);
}

TEST(find_accessibility_element_empty_identifier) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto found = findAccessibilityElement(root, "");
    ASSERT_TRUE(found == nullptr);
}

TEST(find_accessibility_element_dfs_order) {
    // Two children with the same identifier — DFS should return the first one
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto first = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    first->accessibilityIdentifier = "dup";
    auto second = std::make_shared<View>(MakeRect(100, 0, 100, 100));
    second->accessibilityIdentifier = "dup";
    root->addSubview(first);
    root->addSubview(second);

    auto found = findAccessibilityElement(root, "dup");
    ASSERT_TRUE(found == first);
}

// --- convertPointFromWindow ---

TEST(convert_point_from_window_simple) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto child = std::make_shared<View>(MakeRect(10, 20, 100, 100));
    root->addSubview(child);

    Point local = child->convertPointFromWindow(MakePoint(30, 40));
    // Window (30, 40) → child local = (30 - 10, 40 - 20) = (20, 20)
    ASSERT_EQ(local.x, 20);
    ASSERT_EQ(local.y, 20);
}

TEST(convert_point_from_window_nested) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto parent = std::make_shared<View>(MakeRect(10, 20, 200, 200));
    auto child = std::make_shared<View>(MakeRect(5, 5, 100, 100));
    root->addSubview(parent);
    parent->addSubview(child);

    Point local = child->convertPointFromWindow(MakePoint(25, 35));
    // Window (25, 35) → parent local (15, 15) → child local (10, 10)
    ASSERT_EQ(local.x, 10);
    ASSERT_EQ(local.y, 10);
}

TEST(convert_point_from_window_with_scroll) {
    auto root = std::make_shared<View>(MakeRect(0, 0, 480, 800));
    auto scroller = std::make_shared<View>(MakeRect(0, 0, 480, 400));
    auto child = std::make_shared<View>(MakeRect(0, 0, 480, 100));
    root->addSubview(scroller);
    scroller->addSubview(child);

    scroller->setBounds(MakeRect(0, 50, 480, 400));

    // Window point (100, 10): scroller local = (100, 10), with bounds offset adds 50
    // so child local = (100, 10 + 50) = (100, 60)
    Point local = child->convertPointFromWindow(MakePoint(100, 10));
    ASSERT_EQ(local.x, 100);
    ASSERT_EQ(local.y, 60);
}
