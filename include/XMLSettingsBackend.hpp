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
 * @file XMLSettingsBackend.hpp
 * @brief File-based XML settings backend for the desktop emulator.
 *
 * XMLSettingsBackend stores key-value settings in a TinyXML2 document on disk.
 * It is used by the emulator build where NVS (ESP32 non-volatile storage) is
 * not available. The file is loaded on construction and saved after every write.
 */

#pragma once

#include "SettingsBackend.hpp"
#include <string>
#include <memory>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

/**
 * @brief SettingsBackend that persists values to an XML file on disk.
 *
 * Each key is stored as an XML element with a type attribute. The file is
 * created automatically if it doesn't exist, and is rewritten after every
 * set/erase operation.
 */
class XMLSettingsBackend : public SettingsBackend {
public:
    /**
     * @brief Construct an XML settings backend.
     * @param filePath Path to the XML settings file (created if missing).
     */
    XMLSettingsBackend(const std::string& filePath);
    ~XMLSettingsBackend();

    bool hasKey(const std::string& key) override;
    void eraseKey(const std::string& key) override;

    std::string getString(const std::string& key) override;
    void setString(const std::string& key, const std::string& value) override;

    int32_t getInt(const std::string& key) override;
    void setInt(const std::string& key, int32_t value) override;

    bool getBool(const std::string& key) override;
    void setBool(const std::string& key, bool value) override;

private:
    /// @brief Find an existing XML element for a key, or nullptr.
    tinyxml2::XMLElement* findElement(const std::string& key);
    /// @brief Find or create an XML element for a key with the given type.
    tinyxml2::XMLElement* findOrCreateElement(const std::string& key, const char* typeName);
    /// @brief Write the XML document to disk.
    void save();

    std::string filePath;                          ///< Path to the settings file.
    std::unique_ptr<tinyxml2::XMLDocument> doc;    ///< The in-memory XML document.
};
