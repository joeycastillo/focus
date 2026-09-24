#include "viewcontrollers/GalleryViewController.hpp"
#include "viewcontrollers/AboutViewController.hpp"

#include "Application.hpp"
#include "StackView.hpp"
#include "LabelView.hpp"
#include "Button.hpp"
#include "Checkbox.hpp"
#include "Slider.hpp"
#include "NavigationViewController.hpp"
#include "Focus.hpp"

using namespace focus;

GalleryViewController::GalleryViewController(std::shared_ptr<Application> application) : ViewController(application) {
    this->setTitle("Focus");
}

void GalleryViewController::createView() {
    this->view = std::make_shared<View>(RectZero);
    this->view->setOpaque(false);

    auto stack = std::make_shared<VStack>(RectZero);
    stack->setMargins(2);
    stack->setSpacing(2);

    auto sayHi = std::make_shared<Button>(MakeRect(0, 0, 0, 20), "Say hi");
    sayHi->setAction([this](Event, std::weak_ptr<View>) {
        this->greeted = !this->greeted;
        this->statusLabel->setText(this->greeted ? "Hi from SAMD51!" : "");
    }, FOCUS_EVENT_SELECT, this->weak_from_this());
    stack->addSubview(sayHi);

    // This view is rebuilt each time the screen reappears, so the controls
    // start from state kept here in the controller, and report changes back.
    auto checkbox = std::make_shared<Checkbox>(MakeRect(0, 0, 0, 20), "Checkbox");
    checkbox->setSelected(this->checked);
    checkbox->setAction([this](Event, std::weak_ptr<View> sender) {
        if (auto box = std::dynamic_pointer_cast<Checkbox>(sender.lock())) {
            this->checked = box->isSelected();
        }
    }, FOCUS_EVENT_VALUE_CHANGED, this->weak_from_this());
    stack->addSubview(checkbox);

    auto slider = std::make_shared<Slider>(MakeRect(0, 0, 0, 20), "Value:");
    slider->setValue(this->sliderValue);
    slider->setAction([this](Event, std::weak_ptr<View> sender) {
        if (auto s = std::dynamic_pointer_cast<Slider>(sender.lock())) {
            this->sliderValue = s->getValue();
        }
    }, FOCUS_EVENT_VALUE_CHANGED, this->weak_from_this());
    stack->addSubview(slider);

    auto about = std::make_shared<Button>(MakeRect(0, 0, 0, 20), "About");
    about->setAction([this](Event, std::weak_ptr<View>) {
        auto app = this->application.lock();
        auto nav = this->getNavigationController();
        if (app && nav) {
            nav->pushViewController(std::make_shared<AboutViewController>(app));
        }
    }, FOCUS_EVENT_SELECT, this->weak_from_this());
    stack->addSubview(about);

    this->statusLabel = std::make_shared<LabelView>(MakeRect(0, 0, 0, 10),
        this->greeted ? "Hi from SAMD51!" : "");
    stack->addSubview(this->statusLabel);

    this->view->addSubview(stack);
    this->stack = stack;
}

void GalleryViewController::viewDidLayoutSubviews() {
    if (this->stack) {
        this->stack->setFrame(MakeRect(0, 0,
            this->view->getFrame().size.width,
            this->view->getFrame().size.height));
    }
}
