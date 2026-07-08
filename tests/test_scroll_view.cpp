/*
 * Tests for ScrollView: offset clamping, same-offset no-op, re-clamping on
 * frame and content-size changes, and clipped scrolled drawing.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "Window.hpp"
#include "ScrollView.hpp"

using namespace focus;

TEST(scroll_offset_clamps_to_content) {
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 0, 100, 100));
    sv->setContentSize(MakeSize(100, 350));

    sv->setScrollOffset(MakePoint(0, -50));
    ASSERT_EQ(sv->getScrollOffset().y, 0);

    sv->setScrollOffset(MakePoint(0, 10000));
    ASSERT_EQ(sv->getScrollOffset().y, 250);  // 350 - 100

    sv->setScrollOffset(MakePoint(-5, 120));
    ASSERT_EQ(sv->getScrollOffset().x, 0);
    ASSERT_EQ(sv->getScrollOffset().y, 120);
}

TEST(scroll_offset_pins_when_content_fits) {
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 0, 100, 100));
    sv->setContentSize(MakeSize(50, 50));
    sv->setScrollOffset(MakePoint(30, 30));
    ASSERT_EQ(sv->getScrollOffset().x, 0);
    ASSERT_EQ(sv->getScrollOffset().y, 0);
}

TEST(scroll_default_content_size_is_inert) {
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 0, 100, 100));
    sv->setScrollOffset(MakePoint(0, 40));  // no content size set
    ASSERT_EQ(sv->getScrollOffset().y, 0);
}

TEST(scroll_same_offset_does_not_invalidate) {
    std::shared_ptr<RecordingDisplay> display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 100, 480, 400));
    sv->setContentSize(MakeSize(480, 1000));
    window->addSubview(sv);
    sv->setScrollOffset(MakePoint(0, 600));  // clamps to 600
    window->clearNeedsDisplay();

    sv->setScrollOffset(MakePoint(0, 9999));  // clamps to 600 again: no-op
    ASSERT_FALSE(window->needsDisplay());

    sv->setScrollOffset(MakePoint(0, 300));   // real change: invalidates viewport
    ASSERT_TRUE(window->needsDisplay());
    Rect dirty = window->getDirtyRect();
    ASSERT_EQ(dirty.origin.y, 100);
    ASSERT_EQ(dirty.size.height, 400);
}

TEST(scroll_reclamps_on_frame_and_content_changes) {
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 0, 100, 100));
    sv->setContentSize(MakeSize(100, 400));
    sv->setScrollOffset(MakePoint(0, 300));
    ASSERT_EQ(sv->getScrollOffset().y, 300);

    sv->setContentSize(MakeSize(100, 250));   // content shrank
    ASSERT_EQ(sv->getScrollOffset().y, 150);  // 250 - 100

    sv->setFrame(MakeRect(0, 0, 100, 200));   // viewport grew
    ASSERT_EQ(sv->getScrollOffset().y, 50);   // 250 - 200
}

TEST(scroll_view_clips_by_default) {
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 0, 100, 100));
    ASSERT_TRUE(sv->getClipsToBounds());
}

TEST(scroll_view_draws_scrolled_content_clipped) {
    auto display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    auto sv = std::make_shared<ScrollView>(MakeRect(0, 100, 480, 400));
    sv->setContentSize(MakeSize(480, 1000));
    auto content = std::make_shared<View>(MakeRect(0, 0, 480, 1000));
    content->setBackgroundColor(0x0002);
    auto marker = std::make_shared<View>(MakeRect(0, 600, 480, 10));
    marker->setBackgroundColor(0x0003);
    window->addSubview(sv);
    sv->addSubview(content);
    content->addSubview(marker);
    sv->setScrollOffset(MakePoint(0, 550));
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    // Marker at content y=600, offset 550, viewport top 100: screen y 150..160.
    ASSERT_EQ(display->pixel(240, 149), 0x02);
    ASSERT_EQ(display->pixel(240, 155), 0x03);
    ASSERT_EQ(display->pixel(240, 160), 0x02);
    // Nothing from the scroller paints outside rows [100, 500).
    ASSERT_EQ(display->pixel(240, 99), 0xFF);
    ASSERT_EQ(display->pixel(240, 500), 0xFF);
}
