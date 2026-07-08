// tests/test_invalidation.cpp
/*
 * Tests for dirty-rect invalidation coordinate math, including scrolled
 * containers (nonzero bounds.origin).
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "Window.hpp"
#include "View.hpp"

using namespace focus;

static std::shared_ptr<Window> makeWindow(std::shared_ptr<RecordingDisplay>& display,
                                          int w = 480, int h = 800) {
    display = std::make_shared<RecordingDisplay>(w, h);
    auto window = std::make_shared<Window>(display, MakeSize(w, h));
    window->clearNeedsDisplay();  // discard the construction-time dirty rect
    return window;
}

TEST(invalidation_basic_offset_container) {
    std::shared_ptr<RecordingDisplay> display;
    auto window = makeWindow(display);
    auto container = std::make_shared<View>(MakeRect(10, 20, 200, 300));
    auto child = std::make_shared<View>(MakeRect(5, 5, 50, 50));
    window->addSubview(container);
    container->addSubview(child);
    window->clearNeedsDisplay();

    child->setHidden(true);  // invalidates the child's frame

    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.x, 15);
    ASSERT_EQ(dirty.origin.y, 25);
    ASSERT_EQ(dirty.size.width, 50);
    ASSERT_EQ(dirty.size.height, 50);
}

TEST(invalidation_inside_scrolled_container) {
    std::shared_ptr<RecordingDisplay> display;
    auto window = makeWindow(display);
    auto scroller = std::make_shared<View>(MakeRect(0, 100, 480, 400));
    auto content = std::make_shared<View>(MakeRect(0, 0, 480, 1000));
    auto widget = std::make_shared<View>(MakeRect(10, 300, 50, 50));
    window->addSubview(scroller);
    scroller->addSubview(content);
    content->addSubview(widget);
    scroller->setBounds(MakeRect(0, 250, 480, 400));  // scrolled down 250
    window->clearNeedsDisplay();

    widget->setHidden(true);

    // widget at content y=300; scroller shows content y 250..650 at screen
    // y 100..500, so the widget's screen position is y = 100 + (300-250) = 150.
    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.x, 10);
    ASSERT_EQ(dirty.origin.y, 150);
    ASSERT_EQ(dirty.size.width, 50);
    ASSERT_EQ(dirty.size.height, 50);
}

TEST(add_subview_invalidates_in_offset_container) {
    // Regression for commit 7dcd82e, which shipped untested.
    std::shared_ptr<RecordingDisplay> display;
    auto window = makeWindow(display);
    auto container = std::make_shared<View>(MakeRect(0, 100, 480, 400));
    window->addSubview(container);
    window->clearNeedsDisplay();

    container->addSubview(std::make_shared<View>(MakeRect(10, 20, 50, 50)));

    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.x, 10);
    ASSERT_EQ(dirty.origin.y, 120);
    ASSERT_EQ(dirty.size.width, 50);
    ASSERT_EQ(dirty.size.height, 50);
}

TEST(add_subview_invalidates_in_scrolled_container) {
    std::shared_ptr<RecordingDisplay> display;
    auto window = makeWindow(display);
    auto scroller = std::make_shared<View>(MakeRect(0, 100, 480, 400));
    window->addSubview(scroller);
    scroller->setBounds(MakeRect(0, 250, 480, 400));
    window->clearNeedsDisplay();

    scroller->addSubview(std::make_shared<View>(MakeRect(10, 300, 50, 50)));

    // Child at scroller-content y=300, scroll offset 250, scroller at y=100:
    // screen y = 100 + 300 - 250 = 150.
    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.y, 150);
    ASSERT_EQ(dirty.size.height, 50);
}

TEST(remove_subview_invalidates_in_scrolled_container) {
    std::shared_ptr<RecordingDisplay> display;
    auto window = makeWindow(display);
    auto scroller = std::make_shared<View>(MakeRect(0, 100, 480, 400));
    auto child = std::make_shared<View>(MakeRect(10, 300, 50, 50));
    window->addSubview(scroller);
    scroller->addSubview(child);
    scroller->setBounds(MakeRect(0, 250, 480, 400));
    window->clearNeedsDisplay();

    scroller->removeSubview(child);

    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.y, 150);
    ASSERT_EQ(dirty.size.height, 50);
}
