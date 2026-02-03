/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "AlertViewController.hpp"
#include "Application.hpp"
#include "Window.hpp"
#include "BorderedView.hpp"
#include "LabelView.hpp"
#include "Button.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"

std::shared_ptr<AlertViewController> AlertViewController::create(
    std::shared_ptr<Application> app,
    std::string title,
    std::string message,
    std::vector<std::string> buttonLabels,
    CompletionHandler completion)
{
    auto vc = std::shared_ptr<AlertViewController>(
        new AlertViewController(app, title, message, buttonLabels, completion));
    return vc;
}

AlertViewController::AlertViewController(
    std::shared_ptr<Application> app,
    std::string title,
    std::string message,
    std::vector<std::string> buttonLabels,
    CompletionHandler completion)
    : ViewController(app),
      alertTitle(title),
      alertMessage(message),
      buttonLabels(buttonLabels),
      completion(completion)
{
}

void AlertViewController::onButtonPressed(int index) {
    if (this->completion) {
        this->completion(index);
    }
    if (auto app = this->application.lock()) {
        app->dismissViewController();
    }
}

void AlertViewController::createView() {
    auto app = this->application.lock();
    if (!app) return;

    Size windowSize = app->getWindow()->getFrame().size;

    // Layout constants
    int alertWidth = 400;
    int padding = 20;
    int contentWidth = alertWidth - 2 * padding;
    int buttonHeight = 48;
    int buttonSpacing = 8;
    int sectionSpacing = 16;

    // Resolve fonts
    auto titleFont = Font::withName("timR24");
    auto messageFont = Font::systemFont();

    // Measure title height
    int titleLineHeight = 24;
    int titleTextWidth = 0;
    if (titleFont && titleFont->isValid()) {
        auto provider = titleFont->getSharedGlyphProvider();
        if (provider) {
            titleLineHeight = provider->getGlyphRowCount();
            titleTextWidth = TextLayout::measureTextWidth(
                this->alertTitle.c_str(), 1, provider.get());
        }
    }
    int titleLines = 1;
    if (titleTextWidth > contentWidth && contentWidth > 0) {
        titleLines = (titleTextWidth + contentWidth - 1) / contentWidth;
    }
    int titleHeight = titleLines * titleLineHeight;

    // Measure message height
    int messageLineHeight = 18;
    int messageTextWidth = 0;
    if (messageFont && messageFont->isValid()) {
        auto provider = messageFont->getSharedGlyphProvider();
        if (provider) {
            messageLineHeight = provider->getGlyphRowCount();
            messageTextWidth = TextLayout::measureTextWidth(
                this->alertMessage.c_str(), 1, provider.get());
        }
    }
    int messageLines = 1;
    if (messageTextWidth > contentWidth && contentWidth > 0) {
        messageLines = (messageTextWidth + contentWidth - 1) / contentWidth;
    }
    int messageHeight = messageLines * messageLineHeight;

    // Calculate button layout
    int numButtons = (int)this->buttonLabels.size();
    int totalButtonHeight;
    bool buttonsHorizontal = false;

    if (numButtons <= 2) {
        // Try horizontal layout: buttons share the width
        buttonsHorizontal = true;
        totalButtonHeight = buttonHeight;
    } else {
        // Vertical layout: stack buttons
        totalButtonHeight = numButtons * buttonHeight + (numButtons - 1) * buttonSpacing;
    }

    // Total alert height
    int alertHeight = padding + titleHeight + sectionSpacing
                    + messageHeight + sectionSpacing
                    + totalButtonHeight + padding;

    // Center the alert in the window
    int alertX = (windowSize.width - alertWidth) / 2;
    int alertY = (windowSize.height - alertHeight) / 2;

    // Create the root view (transparent, just for positioning)
    this->view = std::make_shared<View>(MakeRect(0, 0, windowSize.width, windowSize.height));
    this->view->setOpaque(false);

    // Create the bordered alert box
    auto alertBox = std::make_shared<BorderedView>(
        MakeRect(alertX, alertY, alertWidth, alertHeight));
    this->view->addSubview(alertBox);

    int yPos = padding;

    // Title label
    auto titleLabel = std::make_shared<LabelView>(
        MakeRect(padding, yPos, contentWidth, titleHeight),
        this->alertTitle);
    if (titleFont) {
        titleLabel->setFont(titleFont);
    }
    alertBox->addSubview(titleLabel);
    yPos += titleHeight + sectionSpacing;

    // Message label
    auto messageLabel = std::make_shared<LabelView>(
        MakeRect(padding, yPos, contentWidth, messageHeight),
        this->alertMessage);
    alertBox->addSubview(messageLabel);
    yPos += messageHeight + sectionSpacing;

    // Buttons
    if (buttonsHorizontal && numButtons > 0) {
        int totalSpacing = (numButtons - 1) * buttonSpacing;
        int singleButtonWidth = (contentWidth - totalSpacing) / numButtons;

        for (int i = 0; i < numButtons; i++) {
            int buttonX = padding + i * (singleButtonWidth + buttonSpacing);
            auto button = std::make_shared<Button>(
                MakeRect(buttonX, yPos, singleButtonWidth, buttonHeight),
                this->buttonLabels[i]);

            int buttonIndex = i;
            button->setAction(
                [this, buttonIndex](Event, std::weak_ptr<View>) {
                    this->onButtonPressed(buttonIndex);
                },
                FOCUS_EVENT_TOUCH_DOWN);

            alertBox->addSubview(button);
        }
    } else {
        for (int i = 0; i < numButtons; i++) {
            auto button = std::make_shared<Button>(
                MakeRect(padding, yPos, contentWidth, buttonHeight),
                this->buttonLabels[i]);

            int buttonIndex = i;
            button->setAction(
                [this, buttonIndex](Event, std::weak_ptr<View>) {
                    this->onButtonPressed(buttonIndex);
                },
                FOCUS_EVENT_TOUCH_DOWN);

            alertBox->addSubview(button);
            yPos += buttonHeight + buttonSpacing;
        }
    }
}
