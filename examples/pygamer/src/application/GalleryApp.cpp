#include "application/GalleryApp.hpp"

#include "Window.hpp"
#include "NavigationViewController.hpp"
#include "viewcontrollers/GalleryViewController.hpp"

#include "display/DisplayST7735.hpp"
#include "tasks/InputTask.hpp"
#include "tasks/RefreshTask.hpp"

using namespace focus;

GalleryApp::GalleryApp(const std::shared_ptr<Window>& window, std::shared_ptr<DisplayST7735> display) : Application(window), display(display) {}

void GalleryApp::setup() {
    this->addTask(std::make_shared<InputTask>());
    this->addTask(std::make_shared<RefreshTask>(this->display));

    auto gallery = std::make_shared<GalleryViewController>(this->shared_from_this());
    this->setRootViewController(NavigationViewController::create(this->shared_from_this(), gallery));
}
