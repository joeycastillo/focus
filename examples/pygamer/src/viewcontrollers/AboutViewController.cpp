#include "viewcontrollers/AboutViewController.hpp"

#include "Application.hpp"
#include "StackView.hpp"
#include "LabelView.hpp"
#include "Button.hpp"
#include "NavigationViewController.hpp"
#include "Focus.hpp"

using namespace focus;

AboutViewController::AboutViewController(std::shared_ptr<Application> application) : ViewController(application) {
    this->setTitle("About");
}

void AboutViewController::createView() {
    this->view = std::make_shared<View>(RectZero);
    this->view->setOpaque(false);

    auto stack = std::make_shared<VStack>(RectZero);
    stack->setMargins(4);
    stack->setSpacing(2);

    stack->addSubview(std::make_shared<LabelView>(
        MakeRect(0, 0, 0, 10), "Focus on PyGamer"));
    stack->addSubview(std::make_shared<LabelView>(
        MakeRect(0, 0, 0, 10), "SAMD51 / 160x128 ST7735"));

    this->view->addSubview(stack);
    this->stack = stack;
}

void AboutViewController::viewDidLayoutSubviews() {
    if (this->stack) {
        this->stack->setFrame(MakeRect(0, 0,
            this->view->getFrame().size.width,
            this->view->getFrame().size.height));
    }
}
