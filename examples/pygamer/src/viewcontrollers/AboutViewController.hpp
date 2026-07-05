#pragma once

#include "ViewController.hpp"
#include <memory>

namespace focus { class VStack; }

/// About screen: demonstrates push/pop navigation.
class AboutViewController : public focus::ViewController {
public:
    explicit AboutViewController(std::shared_ptr<focus::Application> application);

protected:
    void createView() override;
    void viewDidLayoutSubviews() override;

private:
    std::shared_ptr<focus::VStack> stack;
};
