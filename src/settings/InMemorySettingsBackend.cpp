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

#include "InMemorySettingsBackend.hpp"

bool InMemorySettingsBackend::hasKey(const std::string& key) {
    return store.count(key) > 0;
}

void InMemorySettingsBackend::eraseKey(const std::string& key) {
    store.erase(key);
}

std::string InMemorySettingsBackend::getString(const std::string& key) {
    auto it = store.find(key);
    if (it == store.end()) return "";
    auto* value = std::get_if<std::string>(&it->second);
    return value ? *value : "";
}

void InMemorySettingsBackend::setString(const std::string& key, const std::string& value) {
    store[key] = value;
}

int32_t InMemorySettingsBackend::getInt(const std::string& key) {
    auto it = store.find(key);
    if (it == store.end()) return 0;
    auto* value = std::get_if<int32_t>(&it->second);
    return value ? *value : 0;
}

void InMemorySettingsBackend::setInt(const std::string& key, int32_t value) {
    store[key] = value;
}

bool InMemorySettingsBackend::getBool(const std::string& key) {
    auto it = store.find(key);
    if (it == store.end()) return false;
    auto* value = std::get_if<bool>(&it->second);
    return value ? *value : false;
}

void InMemorySettingsBackend::setBool(const std::string& key, bool value) {
    store[key] = value;
}
