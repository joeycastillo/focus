/*
 * Tests for EdgeDragGestureRecognizer state machine.
 */

#include "test_harness.hpp"
#include "EdgeDragGestureRecognizer.hpp"

using namespace focus;

static Event makeTouch(int32_t type, int x, int y) {
    return {type, (x << 16) | (y & 0xFFFF), 0};
}

TEST(gesture_recognizer_starts_possible) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800));
    // Newly created → Possible state (can accept touches)
    ASSERT_TRUE(gr.wantsTouch(MakePoint(25, 400)));  // inside region
    ASSERT_FALSE(gr.wantsTouch(MakePoint(100, 400))); // outside region
}

TEST(gesture_recognizer_move_beyond_threshold_recognized) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    bool recognized = false;
    gr.onRecognized = [&](Event e) { recognized = true; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    // Move 20px right (exceeds threshold of 15)
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 45, 400));

    ASSERT_TRUE(recognized);
}

TEST(gesture_recognizer_move_below_threshold_stays_possible) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    bool recognized = false;
    gr.onRecognized = [&](Event e) { recognized = true; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    // Move only 10px (below threshold of 15)
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 35, 400));

    ASSERT_FALSE(recognized);
}

TEST(gesture_recognizer_touch_up_while_possible_fails) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    bool recognized = false;
    bool ended = false;
    gr.onRecognized = [&](Event e) { recognized = true; };
    gr.onEnded = [&](Event e) { ended = true; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    // Touch up without moving beyond threshold → Failed (tap detection)
    gr.touchUp(makeTouch(FOCUS_EVENT_TOUCH_UP, 25, 400));

    ASSERT_FALSE(recognized);
    ASSERT_FALSE(ended);  // onEnded not called for Failed state
}

TEST(gesture_recognizer_long_press_while_possible_fails) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    bool recognized = false;
    gr.onRecognized = [&](Event e) { recognized = true; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    gr.longPress(makeTouch(FOCUS_EVENT_LONG_PRESS, 25, 400));

    ASSERT_FALSE(recognized);
}

TEST(gesture_recognizer_continued_moves_after_recognized) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    int moveCount = 0;
    gr.onRecognized = [&](Event e) {};
    gr.onMoved = [&](Event e) { moveCount++; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 45, 400));  // recognized + onMoved
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 55, 400));  // onMoved
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 65, 400));  // onMoved

    ASSERT_EQ(moveCount, 3);
}

TEST(gesture_recognizer_touch_up_after_recognized_calls_ended) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    bool ended = false;
    gr.onRecognized = [&](Event e) {};
    gr.onEnded = [&](Event e) { ended = true; };

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 45, 400));  // recognized
    gr.touchUp(makeTouch(FOCUS_EVENT_TOUCH_UP, 45, 400));

    ASSERT_TRUE(ended);
}

TEST(gesture_recognizer_reset_returns_to_possible) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    gr.onRecognized = [&](Event e) {};

    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 45, 400));  // recognized

    gr.reset();

    // After reset, should be back to Possible and accept new touches
    bool recognized = false;
    gr.onRecognized = [&](Event e) { recognized = true; };
    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));
    gr.touchMoved(makeTouch(FOCUS_EVENT_TOUCH_MOVED, 45, 400));
    ASSERT_TRUE(recognized);
}

TEST(gesture_recognizer_touch_down_point) {
    EdgeDragGestureRecognizer gr(MakeRect(0, 0, 50, 800), 15);
    gr.touchDown(makeTouch(FOCUS_EVENT_TOUCH_DOWN, 25, 400));

    Point p = gr.getTouchDownPoint();
    ASSERT_EQ(p.x, 25);
    ASSERT_EQ(p.y, 400);
}
