/*
 * Tests for the Rect geometry helpers in Focus.hpp.
 */

#include "test_harness.hpp"
#include "Focus.hpp"

using namespace focus;

TEST(rect_intersection_overlap) {
    Rect r = RectIntersection(MakeRect(0, 0, 100, 100), MakeRect(50, 60, 100, 100));
    ASSERT_EQ(r.origin.x, 50);
    ASSERT_EQ(r.origin.y, 60);
    ASSERT_EQ(r.size.width, 50);
    ASSERT_EQ(r.size.height, 40);
}

TEST(rect_intersection_contained) {
    Rect r = RectIntersection(MakeRect(0, 0, 100, 100), MakeRect(10, 20, 30, 40));
    ASSERT_TRUE(RectsEqual(r, MakeRect(10, 20, 30, 40)));
}

TEST(rect_intersection_identical) {
    Rect a = MakeRect(5, 5, 10, 10);
    ASSERT_TRUE(RectsEqual(RectIntersection(a, a), a));
}

TEST(rect_intersection_disjoint) {
    Rect r = RectIntersection(MakeRect(0, 0, 10, 10), MakeRect(20, 20, 10, 10));
    ASSERT_TRUE(RectsEqual(r, RectZero));
}

TEST(rect_intersection_edge_touching_is_empty) {
    Rect r = RectIntersection(MakeRect(0, 0, 10, 10), MakeRect(10, 0, 10, 10));
    ASSERT_TRUE(RectsEqual(r, RectZero));
}
