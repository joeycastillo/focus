/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
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
#include "StackView.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"

namespace focus {

std::shared_ptr<AlertViewController> AlertViewController::create(
    std::shared_ptr<Application> app,
    std::string title,
    std::string message,
    std::vector<std::string> buttonLabels,
    CompletionHandler completion)
{
    auto viewController = std::shared_ptr<AlertViewController>(
        new AlertViewController(app, title, message, buttonLabels, completion));
    return viewController;
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

void AlertViewController::createView() {
    auto app = this->application.lock();
    if (!app) return;

    Size windowSize = app->getWindow()->getFrame().size;

    // Derive layout constants from screen size
    int padding = std::max(8, std::min(20, windowSize.width / 24));
    int alertWidth = std::min((int)(windowSize.width * 85 / 100), 400);
    int contentWidth = alertWidth - 2 * padding;
    int buttonSpacing = std::max(4, padding / 3);
    int sectionSpacing = std::max(8, padding * 3 / 4);

    // Resolve fonts
    auto titleFont = Font::systemLargeFont();
    auto messageFont = Font::systemFont();

    // Derive button height from font metrics
    int messageLineHeight = 18;
    if (messageFont && messageFont->isValid()) {
        auto provider = messageFont->getSharedGlyphProvider();
        if (provider) {
            messageLineHeight = provider->getGlyphRowCount();
        }
    }
    int buttonHeight = std::max(24, messageLineHeight + 2 * padding);

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
    int messageTextWidth = 0;
    if (messageFont && messageFont->isValid()) {
        auto provider = messageFont->getSharedGlyphProvider();
        if (provider) {
            messageTextWidth = TextLayout::measureTextWidth(
                this->alertMessage.c_str(), 1, provider.get());
        }
    }
    int messageLines = 1;
    if (messageTextWidth > contentWidth && contentWidth > 0) {
        messageLines = (messageTextWidth + contentWidth - 1) / contentWidth;
    }
    int messageHeight = messageLines * messageLineHeight;

    // Build content stack (title, message, buttons)
    int numButtons = (int)this->buttonLabels.size();
    bool buttonsHorizontal = (numButtons <= 2);

    auto contentStack = std::make_shared<VStack>(RectZero);
    contentStack->setSpacing(sectionSpacing);

    // Title label
    auto titleLabel = std::make_shared<LabelView>(
        MakeRect(0, 0, 0, titleHeight), this->alertTitle);
    if (titleFont) {
        titleLabel->setFont(titleFont);
    }
    titleLabel->accessibilityIdentifier = "alert-title";
    contentStack->addSubview(titleLabel);

    // Message label
    auto messageLabel = std::make_shared<LabelView>(
        MakeRect(0, 0, 0, messageHeight), this->alertMessage);
    messageLabel->accessibilityIdentifier = "alert-message";
    contentStack->addSubview(messageLabel);

    // Buttons
    if (numButtons > 0) {
        if (buttonsHorizontal) {
            auto buttonRow = std::make_shared<HStack>(
                MakeRect(0, 0, 0, buttonHeight));
            buttonRow->setSpacing(buttonSpacing);
            for (int i = 0; i < numButtons; i++) {
                auto button = std::make_shared<Button>(
                    RectZero, this->buttonLabels[i]);
                button->accessibilityIdentifier = "alert-button-" + std::to_string(i);
                int buttonIndex = i;
                auto weakApp = this->application;
                auto completion = this->completion;
                button->setAction(
                    [weakApp, completion, buttonIndex](Event, std::weak_ptr<View>) {
                        if (auto app = weakApp.lock()) {
                            app->dismissViewController();
                        }
                        if (completion) {
                            completion(buttonIndex);
                        }
                    },
                    FOCUS_EVENT_TOUCH_UP_INSIDE);
                buttonRow->addSubview(button);
            }
            contentStack->addSubview(buttonRow);
        } else {
            int verticalButtonHeight = numButtons * buttonHeight
                + (numButtons - 1) * buttonSpacing;
            auto buttonStack = std::make_shared<VStack>(
                MakeRect(0, 0, 0, verticalButtonHeight));
            buttonStack->setSpacing(buttonSpacing);
            for (int i = 0; i < numButtons; i++) {
                auto button = std::make_shared<Button>(
                    MakeRect(0, 0, 0, buttonHeight), this->buttonLabels[i]);
                button->accessibilityIdentifier = "alert-button-" + std::to_string(i);
                int buttonIndex = i;
                auto weakApp = this->application;
                auto completion = this->completion;
                button->setAction(
                    [weakApp, completion, buttonIndex](Event, std::weak_ptr<View>) {
                        if (auto app = weakApp.lock()) {
                            app->dismissViewController();
                        }
                        if (completion) {
                            completion(buttonIndex);
                        }
                    },
                    FOCUS_EVENT_TOUCH_UP_INSIDE);
                buttonStack->addSubview(button);
            }
            contentStack->addSubview(buttonStack);
        }
    }

    // Calculate total alert height
    int buttonSectionHeight = (numButtons == 0) ? 0
        : buttonsHorizontal ? buttonHeight
        : (numButtons * buttonHeight + (numButtons - 1) * buttonSpacing);
    int contentHeight = titleHeight + sectionSpacing
        + messageHeight + sectionSpacing + buttonSectionHeight;
    int alertHeight = 2 * padding + contentHeight;

    // Center the alert in the window
    int alertX = (windowSize.width - alertWidth) / 2;
    int alertY = (windowSize.height - alertHeight) / 2;

    // Create the root view (transparent, just for positioning)
    this->view = std::make_shared<View>(
        MakeRect(0, 0, windowSize.width, windowSize.height));
    this->view->setOpaque(false);

    // Create the bordered alert box
    auto alertBox = std::make_shared<BorderedView>(
        MakeRect(alertX, alertY, alertWidth, alertHeight));
    this->view->addSubview(alertBox);

    // Position the content stack inside the alert box
    contentStack->setFrame(
        MakeRect(padding, padding, contentWidth, contentHeight));
    alertBox->addSubview(contentStack);
}

}  // namespace focus
