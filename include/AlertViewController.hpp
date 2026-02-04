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

/**
 * @file AlertViewController.hpp
 * @brief A modal dialog with a title, message, and action buttons.
 *
 * AlertViewController presents a centered dialog box with a title, a message
 * body, and one or more buttons. When a button is pressed, the dialog is
 * dismissed and the completion handler is called with the button index.
 *
 * Use the static create() factory method rather than constructing directly.
 */

#pragma once

#include "ViewController.hpp"
#include <string>
#include <vector>
#include <functional>

/**
 * @brief A modal alert dialog with buttons.
 *
 * Present via Application::presentViewController(). The dialog auto-dismisses
 * when any button is pressed.
 */
class AlertViewController : public ViewController {
public:
    /// @brief Callback invoked when a button is pressed, with the button's index.
    using CompletionHandler = std::function<void(int buttonIndex)>;

    /**
     * @brief Create an alert dialog.
     * @param app The application (for modal presentation).
     * @param title Bold title text at the top of the dialog.
     * @param message Body text below the title.
     * @param buttonLabels Labels for each button, displayed left to right.
     * @param completion Optional callback invoked with the pressed button's index.
     * @return A shared_ptr to the AlertViewController, ready for presentation.
     */
    static std::shared_ptr<AlertViewController> create(
        std::shared_ptr<Application> app,
        std::string title,
        std::string message,
        std::vector<std::string> buttonLabels,
        CompletionHandler completion = nullptr);

protected:
    AlertViewController(std::shared_ptr<Application> app,
                        std::string title,
                        std::string message,
                        std::vector<std::string> buttonLabels,
                        CompletionHandler completion);
    void createView() override;

private:
    std::string alertTitle;
    std::string alertMessage;
    std::vector<std::string> buttonLabels;
    CompletionHandler completion;

    void onButtonPressed(int index);
};
