#pragma once

#include "ViewController.hpp"
#include <memory>

namespace focus {
class VStack;
class LabelView;
}

/// Gallery: demonstrates Button, Checkbox and Slider controls, plus a
/// status label and an About button that pushes AboutViewController.
class GalleryViewController : public focus::ViewController {
public:
    explicit GalleryViewController(std::shared_ptr<focus::Application> application);

protected:
    void createView() override;
    void viewDidLayoutSubviews() override;

private:
    std::shared_ptr<focus::VStack> stack;
    std::shared_ptr<focus::LabelView> statusLabel;
    bool greeted = false;
    bool checked = false;
    float sliderValue = 0.5f;
};
