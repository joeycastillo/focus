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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <type_traits>

namespace focus {

/**
 * Locale provides a Font-style cached factory for loading localized string tables.
 * Locale files are simple key=value text files (UTF-8, .strings extension).
 *
 * Usage:
 *   // At app startup
 *   Locale::addLocaleSearchPath("/system/locale/");
 *   Locale::setDefaultLocale(Locale::withIdentifier("en"));
 *   Locale::setCurrentLocale(Locale::withIdentifier("en"));
 *
 *   // In views — the comment is both documentation and fallback
 *   label->setText(_LS("wifi_settings", "WiFi Settings"));
 *   std::string page = _LF("page_of", "Page {0} of {1}", current, total);
 *
 * @par Memory management
 * The locale cache grows without bound as new locales are loaded.
 * On memory-constrained devices, call clearCache() when the platform
 * signals memory pressure, or at natural transition points (e.g.,
 * after changing the active locale). clearCache() preserves the
 * active and fallback locales; all other cached locales are freed.
 * Any Locale* previously obtained via withIdentifier() (other than
 * the active/fallback) is invalidated by clearCache().
 * @ingroup locale
 */
class Locale {
public:
    /**
     * Load a locale by identifier. Returns cached instance if already loaded.
     * Searches all registered paths for a file named "{identifier}.strings".
     * @param identifier Locale identifier (e.g., "en", "es", "fr")
     */
    static Locale* withIdentifier(const std::string& identifier);

    /**
     * Set the active locale for the application.
     * Posts a "LocaleChanged" notification via NotificationCenter.
     */
    static void setCurrentLocale(Locale* locale);

    /// Get the current active locale.
    static Locale* currentLocale();

    /// Set the default/fallback locale (used when a key is missing from the active locale).
    static void setDefaultLocale(Locale* locale);

    /// Get the default/fallback locale.
    static Locale* defaultLocale();

    /**
     * Set the locale search path (clears any existing paths).
     * @param path Directory path (e.g., "/system/locale/")
     */
    static void setLocaleSearchPath(const std::string& path);

    /**
     * Add an additional search path for locale files.
     * Paths are searched in the order they are added.
     * @param path Directory path (e.g., "/system/locale/")
     */
    static void addLocaleSearchPath(const std::string& path);

    /// Clear all locale search paths.
    static void clearSearchPaths();

    /// Get the current list of search paths.
    static const std::vector<std::string>& getSearchPaths();

    /**
     * Clear the locale cache, freeing all cached locales except the
     * active and fallback locales (which are preserved and re-added
     * to the cache).
     *
     * Any Locale* previously obtained via withIdentifier() — other
     * than currentLocale() and defaultLocale() — is invalidated.
     * In practice this rarely matters, since application code
     * accesses locales through _LS / _LF / _LP, which look up
     * currentLocale() on every call.
     *
     * Call this from your platform's memory-pressure handler, or
     * after changing the active locale, to free unused locale data.
     */
    static void clearCache();

    /**
     * Look up a string by key in this locale.
     * @param key The string key (e.g., "wifi_settings")
     * @return The localized string, or empty string if not found.
     */
    std::string getString(const std::string& key) const;

    /// Get the locale identifier (e.g., "en", "es").
    const std::string& getIdentifier() const;

    /// Whether this locale was successfully loaded.
    bool isValid() const;

private:
    Locale(const std::string& identifier, std::map<std::string, std::string> strings);

    std::string identifier;
    std::map<std::string, std::string> strings;
    bool valid = false;

    static std::map<std::string, Locale*> localeCache;
    static std::vector<std::string> searchPaths;
    static Locale* activeLocale;
    static Locale* fallbackLocale;

    static Locale* loadLocaleFile(const std::string& identifier);
    static std::map<std::string, std::string> parseFile(const std::string& path);
};

// ---------------------------------------------------------------------------
// Convenience free functions
// ---------------------------------------------------------------------------

namespace detail {
    inline std::string toString(const std::string& v) { return v; }
    inline std::string toString(const char* v) { return v ? std::string(v) : std::string(); }
    template<size_t N>
    inline std::string toString(const char (&v)[N]) { return std::string(v); }
    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    inline std::string toString(const T& v) { return std::to_string(v); }

    inline std::string substitute(const std::string& templ, const std::vector<std::string>& args) {
        std::string result;
        result.reserve(templ.size());
        for (size_t i = 0; i < templ.size(); ++i) {
            if (templ[i] == '{' && i + 2 < templ.size() && templ[i + 2] == '}' &&
                templ[i + 1] >= '0' && templ[i + 1] <= '9') {
                size_t idx = templ[i + 1] - '0';
                if (idx < args.size()) {
                    result += args[idx];
                }
                i += 2;
            } else {
                result += templ[i];
            }
        }
        return result;
    }
}

/**
 * Look up a localized string by key. The comment serves as documentation and
 * as the fallback if no locale is loaded or the key is missing.
 * @param key The string key (e.g., "wifi_settings")
 * @param comment The English text / fallback (e.g., "WiFi Settings")
 */
inline std::string _LS(const std::string& key, const std::string& comment) {
    auto locale = Locale::currentLocale();
    if (locale) {
        std::string result = locale->getString(key);
        if (!result.empty()) return result;
    }
    auto fallback = Locale::defaultLocale();
    if (fallback && fallback != locale) {
        std::string result = fallback->getString(key);
        if (!result.empty()) return result;
    }
    return comment;
}

/**
 * Look up and format a localized string with positional arguments.
 * The comment serves as documentation and as the fallback format string.
 * @param key The string key (e.g., "page_of")
 * @param comment The English format string (e.g., "Page {0} of {1}")
 * @param args Values to substitute for {0}, {1}, etc.
 */
template<typename... Args>
inline std::string _LF(const std::string& key, const std::string& comment, Args&&... args) {
    std::vector<std::string> argVec;
    (argVec.push_back(detail::toString(std::forward<Args>(args))), ...);

    auto locale = Locale::currentLocale();
    if (locale) {
        std::string templ = locale->getString(key);
        if (!templ.empty()) return detail::substitute(templ, argVec);
    }
    auto fallback = Locale::defaultLocale();
    if (fallback && fallback != locale) {
        std::string templ = fallback->getString(key);
        if (!templ.empty()) return detail::substitute(templ, argVec);
    }
    return detail::substitute(comment, argVec);
}

/**
 * Look up a plural form based on count.
 * Looks for key.zero (count==0), key.one (count==1), or key.other.
 * The comment is the fallback format string (use {0} for the count).
 * @param key The base key (e.g., "items")
 * @param comment The English fallback (e.g., "{0} items")
 * @param count The count that determines which plural form to use
 */
inline std::string _LP(const std::string& key, const std::string& comment, int count) {
    std::string subkey;
    if (count == 0) subkey = key + ".zero";
    else if (count == 1) subkey = key + ".one";
    else subkey = key + ".other";

    std::vector<std::string> args = { detail::toString(count) };

    auto locale = Locale::currentLocale();
    if (locale) {
        std::string templ = locale->getString(subkey);
        if (!templ.empty()) return detail::substitute(templ, args);
    }
    auto fallback = Locale::defaultLocale();
    if (fallback && fallback != locale) {
        std::string templ = fallback->getString(subkey);
        if (!templ.empty()) return detail::substitute(templ, args);
    }
    return detail::substitute(comment, args);
}

}  // namespace focus
