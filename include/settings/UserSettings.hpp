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
 * @file UserSettings.hpp
 * @brief Platform-agnostic key-value settings store with namespacing.
 *
 * UserSettings provides typed get/set operations (string, int, bool) backed
 * by a pluggable SettingsBackend. The storage backing is platform-agnostic;
 * an in-memory backend and an XML backend are provided, and applications
 * can implement SettingsBackend for their platform's persistent storage.
 *
 * Instances are accessed by namespace (e.g. "MyApp") and cached — repeated
 * calls with the same namespace return the same instance. The backend factory
 * must be registered once at startup via setBackendFactory().
 *
 * Usage:
 * @code
 *   // At startup
 *   UserSettings::setBackendFactory([](const std::string& ns) {
 *       return std::make_unique<NVSSettingsBackend>(ns);
 *   });
 *
 *   // Anywhere in the app
 *   auto settings = UserSettings::withNamespace("MyApp");
 *   std::string font = settings->getString("my_setting");
 * @endcode
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

class SettingsBackend;

/**
 * @brief Namespaced key-value settings store with a pluggable backend.
 *
 * Non-copyable singleton-per-namespace. Delegates all storage operations
 * to a SettingsBackend instance created by the registered factory.
 */
class UserSettings {
public:
    /// @brief Factory function type that creates a SettingsBackend for a given namespace.
    using BackendFactory = std::function<std::unique_ptr<SettingsBackend>(const std::string&)>;

    /**
     * @brief Register the factory used to create SettingsBackend instances.
     *
     * Must be called once at startup before any withNamespace() calls.
     * @param factory A function that creates a backend for a given namespace name.
     */
    static void setBackendFactory(BackendFactory factory);

    /**
     * @brief Get or create a UserSettings instance for the given namespace.
     *
     * Returns the same instance on repeated calls with the same name.
     * @param name The namespace (e.g. "MyApp").
     * @return Pointer to the cached UserSettings instance.
     */
    static UserSettings* withNamespace(const std::string& name);

    /// @brief Check whether a key exists in this namespace.
    bool hasKey(const std::string& key) const;

    /// @brief Get a string value. If the key doesn't exist, the backend should return an empty string.
    std::string getString(const std::string& key) const;
    /// @brief Set a string value.
    void setString(const std::string& key, const std::string& value);

    /// @brief Get a 32-bit integer value. If the key doesn't exist, the backend should return 0.
    int32_t getInt(const std::string& key) const;
    /// @brief Set a 32-bit integer value.
    void setInt(const std::string& key, int32_t value);

    /// @brief Get a boolean value. If the key doesn't exist, the backend should return false.
    bool getBool(const std::string& key) const;
    /// @brief Set a boolean value.
    void setBool(const std::string& key, bool value);

    /// @brief Delete a key and its value from storage.
    void eraseKey(const std::string& key);

    UserSettings(const UserSettings&) = delete;
    void operator=(const UserSettings&) = delete;

private:
    UserSettings(std::unique_ptr<SettingsBackend> backend);

    std::unique_ptr<SettingsBackend> backend; ///< Platform-specific storage backend.

    static BackendFactory backendFactory;               ///< Registered backend factory.
    static std::map<std::string, UserSettings*> instances; ///< Cached instances by namespace.
};
