// tests/test_clipping.cpp
/*
 * Tests for clipsToBounds: the draw pass narrows its clip rect to a clipping
 * view's on-screen frame, in both zero-size-clip directions.
 */

#include "test_harness.hpp"
#include "RecordingDisplay.hpp"
#include "Window.hpp"
#include "View.hpp"

using namespace focus;

// Window 480x800 with a clipping scroller at (0,100,480,400) holding an
// opaque content child (0,0,480,1000), scrolled down 250. Content pixels
// composite as 0x01; window background as 0xFF.
struct ClipFixture {
    std::shared_ptr<RecordingDisplay> display;
    std::shared_ptr<Window> window;
    std::shared_ptr<View> scroller;
    std::shared_ptr<View> content;
};

static ClipFixture makeClipFixture() {
    ClipFixture f;
    f.display = std::make_shared<RecordingDisplay>(480, 800);
    f.window = std::make_shared<Window>(f.display, MakeSize(480, 800));
    f.scroller = std::make_shared<View>(MakeRect(0, 100, 480, 400));
    f.scroller->setClipsToBounds(true);
    f.content = std::make_shared<View>(MakeRect(0, 0, 480, 1000));
    f.content->setBackgroundColor(0x0001);
    f.window->addSubview(f.scroller);
    f.scroller->addSubview(f.content);
    f.scroller->setBounds(MakeRect(0, 250, 480, 400));
    f.window->clearNeedsDisplay();
    f.display->reset();
    return f;
}

TEST(clips_to_bounds_contains_overflow_on_full_redraw) {
    ClipFixture f = makeClipFixture();
    f.window->draw(0, 0, MakeRect(0, 0, 480, 800));  // full-window dirty rect

    // Content fills exactly the viewport rows [100, 500); window bg elsewhere.
    ASSERT_EQ(f.display->pixel(240, 99), 0xFF);
    ASSERT_EQ(f.display->pixel(240, 100), 0x01);
    ASSERT_EQ(f.display->pixel(240, 499), 0x01);
    ASSERT_EQ(f.display->pixel(240, 500), 0xFF);
}

TEST(clips_to_bounds_replaces_zero_size_clip) {
    ClipFixture f = makeClipFixture();
    f.window->draw(0, 0);  // zero-size clip = "draw everything"

    ASSERT_EQ(f.display->pixel(240, 99), 0xFF);
    ASSERT_EQ(f.display->pixel(240, 100), 0x01);
    ASSERT_EQ(f.display->pixel(240, 500), 0xFF);
}

TEST(clips_to_bounds_empty_intersection_draws_nothing) {
    ClipFixture f = makeClipFixture();
    f.window->draw(0, 0, MakeRect(0, 700, 480, 50));  // dirty rect misses viewport

    // No content pixel may appear anywhere; empty intersection must not be
    // passed down as a zero-size ("no clipping") rect.
    for (const auto& op : f.display->ops) {
        ASSERT_FALSE(op.color == 0x0001);
    }
    // Viewport row is outside the dirty rect entirely, so nothing paints it;
    // it stays at RecordingDisplay's reset value (0), not window background.
    ASSERT_EQ(f.display->pixel(240, 300), 0x00);
}

TEST(clips_to_bounds_in_offset_ancestor) {
    // Same shape, but the scroller sits inside a container at (20, 50).
    auto display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    auto container = std::make_shared<View>(MakeRect(20, 50, 460, 700));
    auto scroller = std::make_shared<View>(MakeRect(0, 100, 400, 300));
    scroller->setClipsToBounds(true);
    auto content = std::make_shared<View>(MakeRect(0, 0, 400, 1000));
    content->setBackgroundColor(0x0001);
    window->addSubview(container);
    container->addSubview(scroller);
    scroller->addSubview(content);
    display->reset();
    window->draw(0, 0, MakeRect(0, 0, 480, 800));

    // Scroller screen rect is (20, 150, 400, 300).
    ASSERT_EQ(display->pixel(200, 149), 0xFF);
    ASSERT_EQ(display->pixel(200, 150), 0x01);
    ASSERT_EQ(display->pixel(200, 449), 0x01);
    ASSERT_EQ(display->pixel(200, 450), 0xFF);
}

TEST(clips_to_bounds_default_off_preserves_overflow) {
    ClipFixture f = makeClipFixture();
    f.scroller->setClipsToBounds(false);
    f.display->reset();
    f.window->draw(0, 0, MakeRect(0, 0, 480, 800));

    // Documented legacy behavior: without clipping, overflow paints below.
    ASSERT_EQ(f.display->pixel(240, 600), 0x01);
}

TEST(clips_to_bounds_zero_size_frame_draws_nothing) {
    // A clipsToBounds view whose own frame has zero area must not fall back
    // to "no clipping" for its children: a zero-size screenRect is the same
    // bit pattern as a zero-size incoming clip, so it must be caught before
    // that "unclipped draw" branch reuses it as the clip rect.
    auto display = std::make_shared<RecordingDisplay>(480, 800);
    auto window = std::make_shared<Window>(display, MakeSize(480, 800));
    auto viewport = std::make_shared<View>(MakeRect(50, 50, 0, 0));
    viewport->setClipsToBounds(true);
    auto child = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    child->setBackgroundColor(0x0004);
    window->addSubview(viewport);
    viewport->addSubview(child);
    display->reset();
    window->draw(0, 0);  // zero-size clip = "draw everything"

    for (const auto& op : display->ops) {
        ASSERT_FALSE(op.color == 0x0004);
    }
    // Window background paints through; the child's color must not.
    ASSERT_EQ(display->pixel(50, 50), 0xFF);
}
