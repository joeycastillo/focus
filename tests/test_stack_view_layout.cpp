/*
 * Tests for StackView layout algorithm.
 */

#include "test_harness.hpp"
#include "StackView.hpp"

using namespace focus;

TEST(stack_view_all_flexible_equal_split) {
    // VStack 300px tall, 3 flexible children → each gets 100px
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 200, 300), StackView::Axis::Vertical);
    auto a = std::make_shared<View>(MakeRect(0, 0, 0, 0));  // size 0 = flexible
    auto b = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto c = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(a);
    stack->addSubview(b);
    stack->addSubview(c);

    ASSERT_EQ(a->getFrame().size.height, 100);
    ASSERT_EQ(b->getFrame().size.height, 100);
    ASSERT_EQ(c->getFrame().size.height, 100);
    ASSERT_EQ(a->getFrame().origin.y, 0);
    ASSERT_EQ(b->getFrame().origin.y, 100);
    ASSERT_EQ(c->getFrame().origin.y, 200);
    // All should fill width
    ASSERT_EQ(a->getFrame().size.width, 200);
}

TEST(stack_view_all_flexible_remainder) {
    // VStack 301px tall, 3 flexible children → 100, 100, 101 (last absorbs remainder)
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 200, 301), StackView::Axis::Vertical);
    auto a = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto b = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto c = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(a);
    stack->addSubview(b);
    stack->addSubview(c);

    ASSERT_EQ(a->getFrame().size.height, 100);
    ASSERT_EQ(b->getFrame().size.height, 100);
    ASSERT_EQ(c->getFrame().size.height, 101);
}

TEST(stack_view_mixed_fixed_flexible) {
    // VStack 300px: fixed child(50px) + 2 flexible → flexible each = (300-50)/2 = 125
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 200, 300), StackView::Axis::Vertical);
    auto fixed = std::make_shared<View>(MakeRect(0, 0, 200, 50));
    auto flex1 = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto flex2 = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(fixed);
    stack->addSubview(flex1);
    stack->addSubview(flex2);

    ASSERT_EQ(fixed->getFrame().size.height, 50);
    ASSERT_EQ(flex1->getFrame().size.height, 125);
    ASSERT_EQ(flex2->getFrame().size.height, 125);
    ASSERT_EQ(fixed->getFrame().origin.y, 0);
    ASSERT_EQ(flex1->getFrame().origin.y, 50);
    ASSERT_EQ(flex2->getFrame().origin.y, 175);
}

TEST(stack_view_spacing) {
    // VStack 300px, 3 flexible children, spacing=10 → spacing between (not at edges)
    // Total spacing = 2 * 10 = 20. Available = 300 - 20 = 280. Each flex = 93, last = 94.
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 200, 300), StackView::Axis::Vertical);
    stack->setSpacing(10);
    auto a = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto b = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto c = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(a);
    stack->addSubview(b);
    stack->addSubview(c);

    ASSERT_EQ(a->getFrame().size.height, 93);
    ASSERT_EQ(b->getFrame().size.height, 93);
    ASSERT_EQ(c->getFrame().size.height, 94);  // last absorbs remainder
    // Check positions account for spacing
    ASSERT_EQ(a->getFrame().origin.y, 0);
    ASSERT_EQ(b->getFrame().origin.y, 103);  // 93 + 10
    ASSERT_EQ(c->getFrame().origin.y, 206);  // 103 + 93 + 10
}

TEST(stack_view_margins) {
    // VStack 300px tall, margins(10, 5, 10, 5), 1 flexible child
    // Available height = 300 - 10 - 10 = 280
    // Available width = 200 - 5 - 5 = 190
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 200, 300), StackView::Axis::Vertical);
    stack->setMargins(10, 5, 10, 5);
    auto child = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(child);

    ASSERT_EQ(child->getFrame().origin.x, 5);
    ASSERT_EQ(child->getFrame().origin.y, 10);
    ASSERT_EQ(child->getFrame().size.width, 190);
    ASSERT_EQ(child->getFrame().size.height, 280);
}

TEST(stack_view_horizontal) {
    // HStack 300px wide, 3 flexible children
    auto stack = std::make_shared<StackView>(MakeRect(0, 0, 300, 100), StackView::Axis::Horizontal);
    auto a = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto b = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    auto c = std::make_shared<View>(MakeRect(0, 0, 0, 0));
    stack->addSubview(a);
    stack->addSubview(b);
    stack->addSubview(c);

    ASSERT_EQ(a->getFrame().size.width, 100);
    ASSERT_EQ(b->getFrame().size.width, 100);
    ASSERT_EQ(c->getFrame().size.width, 100);
    ASSERT_EQ(a->getFrame().origin.x, 0);
    ASSERT_EQ(b->getFrame().origin.x, 100);
    ASSERT_EQ(c->getFrame().origin.x, 200);
    // Height fills the stack
    ASSERT_EQ(a->getFrame().size.height, 100);
}
