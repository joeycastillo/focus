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

/**
 * @file NotificationCenter.hpp
 * @brief Publish-subscribe notification system for decoupled communication.
 *
 * NotificationCenter provides a way for components to communicate without
 * direct references to each other. Observers register callbacks for named
 * notifications, and any component can post a notification to invoke all
 * matching callbacks.
 *
 * Observations can optionally be tied to an owner object's lifetime: when the
 * owner is destroyed, the observation is automatically cleaned up.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <any>

/// @brief A named notification with an optional payload.
struct Notification {
    std::string name;        ///< The notification name (e.g. "dataLoaded").
    /// @brief Optional payload data.
    ///
    /// Prefer small types (integers, pointers) for best performance.
    /// Strings and other types work but may heap-allocate on embedded targets.
    std::any userInfo;
};

/// @brief Callback type for notification observers.
using NotificationCallback = std::function<void(const Notification&)>;

/**
 * @brief Singleton publish-subscribe notification center.
 *
 * Supports re-entrant posting (posting from within a callback) and
 * automatic cleanup of observations tied to destroyed owner objects.
 * @ingroup core
 */
class NotificationCenter {
public:
    /// @brief Get the shared NotificationCenter singleton.
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
    void post(const std::string& name, std::any userInfo = {});

    NotificationCenter(const NotificationCenter&) = delete;
    void operator=(const NotificationCenter&) = delete;

private:
    NotificationCenter() = default;

    /// @brief Internal observer record.
    struct Observer {
        uint32_t token;                 ///< Unique token for this observation.
        std::string name;               ///< Notification name to match.
        NotificationCallback callback;  ///< Callback to invoke.
        std::weak_ptr<void> owner;      ///< Optional owner for lifetime tracking.
        bool hasOwner;                  ///< Whether this observation has a lifetime owner.
    };

    std::vector<Observer> observers;  ///< All registered observers.
    uint32_t nextToken = 1;           ///< Counter for generating unique tokens.
    int postingDepth = 0;             ///< Re-entrancy depth for safe posting.
};
