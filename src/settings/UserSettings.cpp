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

#include "UserSettings.hpp"
#include "SettingsBackend.hpp"
#include "FocusLog.hpp"

UserSettings::BackendFactory UserSettings::backendFactory;
std::map<std::string, UserSettings*> UserSettings::instances;

void UserSettings::setBackendFactory(BackendFactory factory) {
    backendFactory = std::move(factory);
}

UserSettings* UserSettings::withNamespace(const std::string& name) {
    auto it = instances.find(name);
    if (it != instances.end()) {
        return it->second;
    }

    if (!backendFactory) {
        FOCUS_LOGE("UserSettings", "withNamespace called before setBackendFactory");
        return nullptr;
    }

    auto backend = backendFactory(name);
    if (!backend) {
        FOCUS_LOGE("UserSettings", "backendFactory returned null for namespace '%s'", name.c_str());
        return nullptr;
    }

    auto* settings = new UserSettings(std::move(backend));
    instances[name] = settings;
    return settings;
}

UserSettings::UserSettings(std::unique_ptr<SettingsBackend> backend)
    : backend(std::move(backend)) {}

bool UserSettings::hasKey(const std::string& key) const {
    return backend->hasKey(key);
}

std::string UserSettings::getString(const std::string& key) const {
    return backend->getString(key);
}

void UserSettings::setString(const std::string& key, const std::string& value) {
    backend->setString(key, value);
}

int32_t UserSettings::getInt(const std::string& key) const {
    return backend->getInt(key);
}

void UserSettings::setInt(const std::string& key, int32_t value) {
    backend->setInt(key, value);
}

bool UserSettings::getBool(const std::string& key) const {
    return backend->getBool(key);
}

void UserSettings::setBool(const std::string& key, bool value) {
    backend->setBool(key, value);
}

void UserSettings::eraseKey(const std::string& key) {
    backend->eraseKey(key);
}
