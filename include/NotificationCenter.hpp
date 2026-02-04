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

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

struct Notification {
    std::string name;
    int32_t userInfo = 0;
};

using NotificationCallback = std::function<void(const Notification&)>;

class NotificationCenter {
public:
    static NotificationCenter* shared();

    /// Add an observer for notifications with the given name.
    /// If owner is provided, the observation is automatically skipped
    /// (and cleaned up) when the owner is destroyed.
    /// Returns a token that can be used for explicit removal.
    uint32_t addObserver(const std::string& name,
                         NotificationCallback callback,
                         std::shared_ptr<void> owner = nullptr);

    /// Remove an observer by token. Safe to call during a notification callback.
    void removeObserver(uint32_t token);

    /// Post a notification to all observers registered for the given name.
    /// Safe to call re-entrantly (i.e., from within a notification callback).
    void post(const std::string& name, int32_t userInfo = 0);

    NotificationCenter(const NotificationCenter&) = delete;
    void operator=(const NotificationCenter&) = delete;

private:
    NotificationCenter() = default;

    struct Observer {
        uint32_t token;
        std::string name;
        NotificationCallback callback;
        std::weak_ptr<void> owner;
        bool hasOwner;
    };

    std::vector<Observer> observers;
    uint32_t nextToken = 1;
    int postingDepth = 0;
};
