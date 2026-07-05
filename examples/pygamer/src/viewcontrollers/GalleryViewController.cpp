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

    stack->addSubview(std::make_shared<Checkbox>(
        MakeRect(0, 0, 0, 20), "Checkbox"));

    auto slider = std::make_shared<Slider>(MakeRect(0, 0, 0, 20), "Value:");
    slider->setValue(0.5f);
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

    this->statusLabel = std::make_shared<LabelView>(MakeRect(0, 0, 0, 10), "");
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
