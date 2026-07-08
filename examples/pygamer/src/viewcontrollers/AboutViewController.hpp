#pragma once

#include "ViewController.hpp"
#include "View.hpp"
#include "ScrollView.hpp"
#include <memory>

namespace focus { class TextView; }

/// Focusable page wrapper: maps joystick up/down to half-viewport scrolls,
/// declining events at the edges so focus can travel to the nav bar.
class ScrollingPageView : public focus::View {
public:
    ScrollingPageView(focus::Rect rect) : focus::View(rect) {}
    void setScrollView(std::shared_ptr<focus::ScrollView> scrollView) {
        this->scrollView = scrollView;
    }
    bool canBecomeFocused() const override { return true; }
    bool handleEvent(focus::Event event) override;

private:
    std::shared_ptr<focus::ScrollView> scrollView;
};

/// About screen: a long page of text scrolled with the joystick.
class AboutViewController : public focus::ViewController {
public:
    explicit AboutViewController(std::shared_ptr<focus::Application> application);

protected:
    void createView() override;
    void viewDidLayoutSubviews() override;

private:
    std::shared_ptr<ScrollingPageView> page;
    std::shared_ptr<focus::ScrollView> scrollView;
    std::shared_ptr<focus::TextView> textView;
};
