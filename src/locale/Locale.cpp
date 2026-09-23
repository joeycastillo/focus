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
#include "focus_config.h"
#include "FocusLog.hpp"
#include <algorithm>
#include <initializer_list>
#if FOCUS_HAS_FILESYSTEM
#include <fstream>
#include <iterator>
#endif

namespace focus {

static const char *TAG = "Locale";

// Static member initialization
std::map<std::string, Locale*> Locale::localeCache;
std::vector<std::string> Locale::searchPaths;
Locale* Locale::activeLocale = nullptr;
Locale* Locale::fallbackLocale = nullptr;

namespace {

// The first non-empty result of find in the current locale, then the default locale.
template<typename Find>
std::string firstMatch(Find find) {
    Locale* current = Locale::currentLocale();
    if (current) {
        std::string result = find(*current);
        if (!result.empty()) return result;
    }
    Locale* fallback = Locale::defaultLocale();
    if (fallback && fallback != current) {
        std::string result = find(*fallback);
        if (!result.empty()) return result;
    }
    return {};
}

std::map<std::string, PluralRule>& registeredPluralRules() {
    static std::map<std::string, PluralRule> rules;
    return rules;
}

std::string normalizedIdentifier(const std::string& identifier) {
    std::string result = identifier;
    for (char& c : result) {
        if (c == '-') c = '_';
    }
    return result;
}

// "pt_PT" -> "pt"
std::string languagePart(const std::string& normalized) {
    return normalized.substr(0, normalized.find('_'));
}

PluralRule resolvePluralRule(const std::string& identifier) {
    std::string full = normalizedIdentifier(identifier);
    std::string language = languagePart(full);
    const auto& registered = registeredPluralRules();
    for (const std::string* id : {&full, &language}) {
        auto it = registered.find(*id);
        if (it != registered.end()) return it->second;
    }
    for (const std::string* id : {&full, &language}) {
        if (PluralRule rule = detail::builtinPluralRule(*id)) return rule;
    }
    return detail::defaultPluralRule;
}

const char* pluralSuffix(PluralCategory category) {
    switch (category) {
        case PluralCategory::Zero: return ".zero";
        case PluralCategory::One: return ".one";
        case PluralCategory::Two: return ".two";
        case PluralCategory::Few: return ".few";
        case PluralCategory::Many: return ".many";
        case PluralCategory::Other: break;
    }
    return ".other";
}

}  // namespace

std::string detail::lookupString(const std::string& key) {
    return firstMatch([&](const Locale& locale) { return locale.getString(key); });
}

std::string detail::lookupPlural(const std::string& key, int64_t count) {
    return firstMatch([&](const Locale& locale) {
        std::string result = locale.getString(key + pluralSuffix(locale.pluralCategory(count)));
        return result.empty() ? locale.getString(key + ".other") : result;
    });
}

Locale::Locale(const std::string& identifier, std::map<std::string, std::string> strings)
    : identifier(identifier), strings(std::move(strings)), valid(true) {}

Locale* Locale::withIdentifier(const std::string& identifier) {
    auto it = localeCache.find(identifier);
    if (it != localeCache.end()) {
        return it->second;
    }
#if FOCUS_HAS_FILESYSTEM
    auto locale = loadLocaleFile(identifier);
    if (locale) {
        localeCache[identifier] = locale;
        return locale;
    }
#endif
    return nullptr;
}

Locale* Locale::fromMemory(const std::string& identifier, const uint8_t* data, size_t size) {
    auto it = localeCache.find(identifier);
    if (it != localeCache.end()) {
        return it->second;
    }
    auto strings = parseStrings(data, size);
    std::vector<std::string> chain = {identifier};
    mergeParents(strings, chain);
    if (strings.empty()) {
        return nullptr;
    }
    auto* locale = new Locale(identifier, std::move(strings));
    localeCache[identifier] = locale;
    return locale;
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
    for (auto& [id, locale] : localeCache) {
        if (locale != activeLocale && locale != fallbackLocale) {
            delete locale;
        }
    }
    localeCache.clear();
    // Re-add active/fallback so the cache stays consistent
    if (activeLocale) {
        localeCache[activeLocale->getIdentifier()] = activeLocale;
    }
    if (fallbackLocale && fallbackLocale != activeLocale) {
        localeCache[fallbackLocale->getIdentifier()] = fallbackLocale;
    }
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

PluralCategory Locale::pluralCategory(int64_t count) const {
    uint64_t n = count < 0 ? 0 - static_cast<uint64_t>(count) : static_cast<uint64_t>(count);
    return resolvePluralRule(identifier)(n);
}

void Locale::setPluralRule(const std::string& language, PluralRule rule) {
    std::string key = normalizedIdentifier(language);
    if (rule) {
        registeredPluralRules()[key] = rule;
    } else {
        registeredPluralRules().erase(key);
    }
}

std::map<std::string, std::string> Locale::parseStrings(const uint8_t* data, size_t size) {
    std::map<std::string, std::string> result;
    if (!data) return result;

    size_t i = 0;
    while (i < size) {
        size_t start = i;
        while (i < size && data[i] != '\n') ++i;
        std::string line(reinterpret_cast<const char*>(data + start), i - start);
        if (i < size) ++i;  // consume newline

        // Embedded blobs sized with sizeof() carry a trailing NUL; drop it.
        while (!line.empty() && line.back() == '\0') line.pop_back();

        if (line.empty() || line[0] == '#') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        std::string processed;
        processed.reserve(value.size());
        for (size_t j = 0; j < value.size(); ++j) {
            if (value[j] == '\\' && j + 1 < value.size()) {
                switch (value[j + 1]) {
                    case 'n': processed += '\n'; ++j; break;
                    case 't': processed += '\t'; ++j; break;
                    case '\\': processed += '\\'; ++j; break;
                    case '"': processed += '"'; ++j; break;
                    default: processed += value[j]; break;
                }
            } else {
                processed += value[j];
            }
        }
        result[key] = processed;
    }
    return result;
}

void Locale::mergeParents(std::map<std::string, std::string>& strings, std::vector<std::string>& chain) {
    auto declared = strings.find("@parent");
    std::string parent = declared != strings.end() ? declared->second : std::string();
    // Remove directives; they aren't strings.
    for (auto it = strings.begin(); it != strings.end();) {
        if (!it->first.empty() && it->first[0] == '@') {
            it = strings.erase(it);
        } else {
            ++it;
        }
    }
    if (parent.empty()) return;
    if (std::find(chain.begin(), chain.end(), parent) != chain.end()) {
        FOCUS_LOGW(TAG, "%s: parent %s is already in its chain", chain.back().c_str(), parent.c_str());
        return;
    }
    std::map<std::string, std::string> inherited;
    auto cached = localeCache.find(parent);
    if (cached != localeCache.end()) {
        inherited = cached->second->strings;
    } else {
#if FOCUS_HAS_FILESYSTEM
        chain.push_back(parent);
        inherited = readStrings(parent, chain);
        chain.pop_back();
#endif
    }
    if (inherited.empty()) {
        FOCUS_LOGW(TAG, "%s: parent %s not found", chain.back().c_str(), parent.c_str());
        return;
    }
    // The child's strings win; the parent fills in the rest.
    strings.merge(inherited);
}

#if FOCUS_HAS_FILESYSTEM
Locale* Locale::loadLocaleFile(const std::string& identifier) {
    std::vector<std::string> chain = {identifier};
    auto strings = readStrings(identifier, chain);
    if (strings.empty()) return nullptr;
    return new Locale(identifier, std::move(strings));
}

std::map<std::string, std::string> Locale::readStrings(const std::string& identifier, std::vector<std::string>& chain) {
    for (const auto& path : searchPaths) {
        auto strings = parseFile(path + identifier + ".strings");
        if (!strings.empty()) {
            mergeParents(strings, chain);
            return strings;
        }
    }
    return {};
}

std::map<std::string, std::string> Locale::parseFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parseStrings(reinterpret_cast<const uint8_t*>(contents.data()), contents.size());
}
#endif  // FOCUS_HAS_FILESYSTEM

}  // namespace focus
