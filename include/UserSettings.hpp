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
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

class SettingsBackend;

class UserSettings {
public:
    using BackendFactory = std::function<std::unique_ptr<SettingsBackend>(const std::string&)>;

    /// Register a factory that creates SettingsBackend instances.
    /// Must be called once at startup before any withNamespace() calls.
    static void setBackendFactory(BackendFactory factory);

    /// Get or create a UserSettings instance for the given namespace.
    /// Returns the same instance on repeated calls with the same name.
    static UserSettings* withNamespace(const std::string& name);

    bool hasKey(const std::string& key);

    std::string getString(const std::string& key);
    void setString(const std::string& key, const std::string& value);

    int32_t getInt(const std::string& key);
    void setInt(const std::string& key, int32_t value);

    bool getBool(const std::string& key);
    void setBool(const std::string& key, bool value);

    void eraseKey(const std::string& key);

    UserSettings(const UserSettings&) = delete;
    void operator=(const UserSettings&) = delete;

private:
    UserSettings(std::unique_ptr<SettingsBackend> backend);

    std::unique_ptr<SettingsBackend> backend;

    static BackendFactory backendFactory;
    static std::map<std::string, UserSettings*> instances;
};
