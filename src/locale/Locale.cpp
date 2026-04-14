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

#include "Locale.hpp"
#include "NotificationCenter.hpp"
#include <fstream>

// Static member initialization
std::map<std::string, Locale*> Locale::localeCache;
std::vector<std::string> Locale::searchPaths;
Locale* Locale::activeLocale = nullptr;
Locale* Locale::fallbackLocale = nullptr;

Locale::Locale(const std::string& identifier, std::map<std::string, std::string> strings)
    : identifier(identifier), strings(std::move(strings)), valid(true) {}

Locale* Locale::withIdentifier(const std::string& identifier) {
    auto it = localeCache.find(identifier);
    if (it != localeCache.end()) {
        return it->second;
    }

    auto locale = loadLocaleFile(identifier);
    if (locale) {
        localeCache[identifier] = locale;
        return locale;
    }

    return nullptr;
}

void Locale::setCurrentLocale(Locale* locale) {
    activeLocale = locale;
    NotificationCenter::shared()->post("LocaleChanged");
}

Locale* Locale::currentLocale() {
    return activeLocale;
}

void Locale::setDefaultLocale(Locale* locale) {
    fallbackLocale = locale;
}

Locale* Locale::defaultLocale() {
    return fallbackLocale;
}

void Locale::setLocaleSearchPath(const std::string& path) {
    clearSearchPaths();
    addLocaleSearchPath(path);
}

void Locale::addLocaleSearchPath(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p.back() != '/') {
        p += '/';
    }
    searchPaths.push_back(p);
}

void Locale::clearSearchPaths() {
    searchPaths.clear();
}

const std::vector<std::string>& Locale::getSearchPaths() {
    return searchPaths;
}

void Locale::clearCache() {
    localeCache.clear();
}

std::string Locale::getString(const std::string& key) const {
    auto it = strings.find(key);
    if (it != strings.end()) {
        return it->second;
    }
    return {};
}

const std::string& Locale::getIdentifier() const {
    return identifier;
}

bool Locale::isValid() const {
    return valid;
}

Locale* Locale::loadLocaleFile(const std::string& identifier) {
    for (const auto& path : searchPaths) {
        std::string filePath = path + identifier + ".strings";
        auto strings = parseFile(filePath);
        if (!strings.empty()) {
            return new Locale(identifier, std::move(strings));
        }
    }
    return nullptr;
}

std::map<std::string, std::string> Locale::parseFile(const std::string& path) {
    std::map<std::string, std::string> result;
    std::ifstream file(path);
    if (!file.is_open()) return result;

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Find the first '=' separator
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Process escape sequences in value
        std::string processed;
        processed.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '\\' && i + 1 < value.size()) {
                switch (value[i + 1]) {
                    case 'n': processed += '\n'; ++i; break;
                    case 't': processed += '\t'; ++i; break;
                    case '\\': processed += '\\'; ++i; break;
                    default: processed += value[i]; break;
                }
            } else {
                processed += value[i];
            }
        }

        result[key] = processed;
    }

    return result;
}
