/*
 * Tests for NavigationBar slot layout: absent buttons take no space.
 */

#include "test_harness.hpp"
#include "NavigationBar.hpp"
#include "StackView.hpp"
#include "LabelView.hpp"
#include "FocusMetrics.hpp"

using namespace focus;

namespace {

const int kBarWidth = 160;

std::shared_ptr<StackView> stackIn(const std::shared_ptr<NavigationBar>& bar) {
    return std::static_pointer_cast<StackView>(bar->getSubviews()[0]);
}

std::shared_ptr<View> titleIn(const std::shared_ptr<NavigationBar>& bar) {
    for (const auto& child : stackIn(bar)->getSubviews()) {
        if (std::dynamic_pointer_cast<LabelView>(child)) return child;
    }
    return nullptr;
}

int innerWidth() {
    return kBarWidth - 2 * FocusMetrics::get().navBarPadding;
}

}  // namespace

TEST(navigation_bar_no_buttons_title_fills_bar) {
    // At the root of a stack there is no back or right button, and no space
    // is reserved for either: the title spans the bar's full inner width.
    auto bar = NavigationBar::create(kBarWidth);
    auto stack = stackIn(bar);

    ASSERT_EQ((int)stack->getSubviews().size(), 1);
    auto title = titleIn(bar);
    ASSERT_EQ(title->getFrame().origin.x, 0);
    ASSERT_EQ(title->getFrame().size.width, innerWidth());
}

TEST(navigation_bar_back_button_takes_only_its_slot) {
    auto bar = NavigationBar::create(kBarWidth);
    bar->setBackButtonVisible(true);
    auto stack = stackIn(bar);
    int buttonWidth = FocusMetrics::get().navBarButtonWidth;
    int spacing = stack->getSpacing();

    ASSERT_EQ((int)stack->getSubviews().size(), 2);
    auto title = titleIn(bar);
    ASSERT_EQ(title->getFrame().origin.x, buttonWidth + spacing);
    ASSERT_EQ(title->getFrame().size.width, innerWidth() - buttonWidth - spacing);
}

TEST(navigation_bar_back_button_toggle_restores_title) {
    // Round-trip: the title must return to full width after the back button
    // leaves, and to the reduced width when it comes back again.
    auto bar = NavigationBar::create(kBarWidth);
    int buttonWidth = FocusMetrics::get().navBarButtonWidth;
    int spacing = stackIn(bar)->getSpacing();

    bar->setBackButtonVisible(true);
    bar->setBackButtonVisible(false);
    ASSERT_EQ((int)stackIn(bar)->getSubviews().size(), 1);
    ASSERT_EQ(titleIn(bar)->getFrame().origin.x, 0);
    ASSERT_EQ(titleIn(bar)->getFrame().size.width, innerWidth());

    bar->setBackButtonVisible(true);
    ASSERT_EQ(titleIn(bar)->getFrame().size.width, innerWidth() - buttonWidth - spacing);
}

TEST(navigation_bar_right_button_takes_only_its_slot) {
    auto bar = NavigationBar::create(kBarWidth);
    bar->setRightButton("Edit", nullptr);
    auto stack = stackIn(bar);
    int buttonWidth = FocusMetrics::get().navBarButtonWidth;
    int spacing = stack->getSpacing();

    ASSERT_EQ((int)stack->getSubviews().size(), 2);
    auto title = titleIn(bar);
    ASSERT_EQ(title->getFrame().origin.x, 0);
    ASSERT_EQ(title->getFrame().size.width, innerWidth() - buttonWidth - spacing);

    // An empty title removes the button and gives the title its space back.
    bar->setRightButton("", nullptr);
    ASSERT_EQ((int)stackIn(bar)->getSubviews().size(), 1);
    ASSERT_EQ(titleIn(bar)->getFrame().size.width, innerWidth());
}

TEST(navigation_bar_both_buttons_title_between) {
    auto bar = NavigationBar::create(kBarWidth);
    bar->setBackButtonVisible(true);
    bar->setRightButton("Edit", nullptr);
    auto stack = stackIn(bar);
    int buttonWidth = FocusMetrics::get().navBarButtonWidth;
    int spacing = stack->getSpacing();

    ASSERT_EQ((int)stack->getSubviews().size(), 3);
    auto title = titleIn(bar);
    ASSERT_EQ(title->getFrame().origin.x, buttonWidth + spacing);
    ASSERT_EQ(title->getFrame().size.width,
              innerWidth() - 2 * (buttonWidth + spacing));
}
