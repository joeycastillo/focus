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
 * @file SettingsBackend.hpp
 * @brief Abstract interface for persistent key-value storage backends.
 *
 * SettingsBackend defines the storage contract used by UserSettings.
 * Implementations include InMemorySettingsBackend (in-memory, no persistence)
 * and NVSSettingsBackend (ESP32 NVS, in the application layer).
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Abstract interface for typed key-value persistent storage.
 *
 * Implementations must support string, 32-bit integer, and boolean value types,
 * as well as key existence checking and deletion.
 */
class SettingsBackend {
public:
    virtual ~SettingsBackend() = default;

    /// @brief Check whether a key exists in storage.
    virtual bool hasKey(const std::string& key) const = 0;
    /// @brief Remove a key and its value from storage.
    virtual void eraseKey(const std::string& key) = 0;

    /// @brief Read a string value for the given key.
    virtual std::string getString(const std::string& key) const = 0;
    /// @brief Write a string value for the given key.
    virtual void setString(const std::string& key, const std::string& value) = 0;

    /// @brief Read a 32-bit integer value for the given key.
    virtual int32_t getInt(const std::string& key) const = 0;
    /// @brief Write a 32-bit integer value for the given key.
    virtual void setInt(const std::string& key, int32_t value) = 0;

    /// @brief Read a boolean value for the given key.
    virtual bool getBool(const std::string& key) const = 0;
    /// @brief Write a boolean value for the given key.
    virtual void setBool(const std::string& key, bool value) = 0;
};
