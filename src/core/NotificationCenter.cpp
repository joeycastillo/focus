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

#include "NotificationCenter.hpp"
#include <algorithm>
#include <any>

NotificationCenter* NotificationCenter::shared() {
    static NotificationCenter instance;
    return &instance;
}

uint32_t NotificationCenter::addObserver(const std::string& name,
                                          NotificationCallback callback,
                                          std::shared_ptr<void> owner) {
    uint32_t token = nextToken++;
    observers.push_back({token, name, std::move(callback), owner, owner != nullptr});
    return token;
}

void NotificationCenter::removeObserver(uint32_t token) {
    if (postingDepth > 0) {
        // Inside a post() — nullify instead of erasing to keep indices stable.
        for (auto& obs : observers) {
            if (obs.token == token) {
                obs.callback = nullptr;
                break;
            }
        }
    } else {
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                [token](const Observer& obs) { return obs.token == token; }),
            observers.end());
    }
}

void NotificationCenter::post(const std::string& name, std::any userInfo) {
    postingDepth++;

    Notification notification{name, std::move(userInfo)};

    // Iterate by index with snapshotted count: observers added during
    // callbacks are appended beyond this bound and won't fire this pass.
    size_t count = observers.size();
    for (size_t i = 0; i < count; i++) {
        Observer& obs = observers[i];

        // Skip observers removed during this post
        if (!obs.callback) continue;

        // Skip and mark expired owners for cleanup
        if (obs.hasOwner && obs.owner.expired()) {
            obs.callback = nullptr;
            continue;
        }

        if (obs.name == name) {
            obs.callback(notification);
        }
    }

    postingDepth--;

    // Outermost post() sweeps dead entries
    if (postingDepth == 0) {
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                [](const Observer& obs) { return !obs.callback; }),
            observers.end());
    }
}
