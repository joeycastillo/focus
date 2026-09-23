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
#include "focus_config.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace focus {

namespace {

using P = PluralCategory;

#if FOCUS_HAS_PLURAL_RULES

// CLDR 48.2 cardinal plural rules, integer counts only.

bool isMillions(uint64_t n) { return n != 0 && n % 1000000 == 0; }

// Last digit 2..4, except when the last two digits are 12..14.
bool isSlavicFew(uint64_t n) {
    return n % 10 >= 2 && n % 10 <= 4 && !(n % 100 >= 12 && n % 100 <= 14);
}

P noPlural(uint64_t) { return P::Other; }

P oneUpToOne(uint64_t n) { return n <= 1 ? P::One : P::Other; }

P oneMillions(uint64_t n) {
    if (n == 1) return P::One;
    return isMillions(n) ? P::Many : P::Other;
}

P oneUpToOneMillions(uint64_t n) {
    if (n <= 1) return P::One;
    return isMillions(n) ? P::Many : P::Other;
}

P eastSlavic(uint64_t n) {
    if (n % 10 == 1 && n % 100 != 11) return P::One;
    return isSlavicFew(n) ? P::Few : P::Many;
}

P polish(uint64_t n) {
    if (n == 1) return P::One;
    return isSlavicFew(n) ? P::Few : P::Many;
}

P czechSlovak(uint64_t n) {
    if (n == 1) return P::One;
    return n >= 2 && n <= 4 ? P::Few : P::Other;
}

P southSlavic(uint64_t n) {
    if (n % 10 == 1 && n % 100 != 11) return P::One;
    return isSlavicFew(n) ? P::Few : P::Other;
}

P lithuanian(uint64_t n) {
    if (n % 100 >= 11 && n % 100 <= 19) return P::Other;
    if (n % 10 == 1) return P::One;
    return n % 10 >= 2 ? P::Few : P::Other;
}

P latvian(uint64_t n) {
    if (n % 10 == 0 || (n % 100 >= 11 && n % 100 <= 19)) return P::Zero;
    return n % 10 == 1 && n % 100 != 11 ? P::One : P::Other;
}

P arabic(uint64_t n) {
    if (n == 0) return P::Zero;
    if (n == 1) return P::One;
    if (n == 2) return P::Two;
    if (n % 100 >= 3 && n % 100 <= 10) return P::Few;
    if (n % 100 >= 11) return P::Many;
    return P::Other;
}

P oneTwo(uint64_t n) {
    if (n == 1) return P::One;
    return n == 2 ? P::Two : P::Other;
}

P slovenian(uint64_t n) {
    if (n % 100 == 1) return P::One;
    if (n % 100 == 2) return P::Two;
    return n % 100 == 3 || n % 100 == 4 ? P::Few : P::Other;
}

P romanian(uint64_t n) {
    if (n == 1) return P::One;
    if (n == 0 || (n % 100 >= 1 && n % 100 <= 19)) return P::Few;
    return P::Other;
}

P oneEndsInOne(uint64_t n) { return n % 10 == 1 && n % 100 != 11 ? P::One : P::Other; }

struct RuleEntry {
    const char* identifier;
    PluralRule rule;
};

// Sorted by identifier. Omits languages that use the default rule or a rule not implemented here.
static constexpr RuleEntry rules[] = {
    {"ak", oneUpToOne}, {"am", oneUpToOne}, {"ar", arabic}, {"ars", arabic},
    {"as", oneUpToOne}, {"be", eastSlavic}, {"bho", oneUpToOne}, {"bm", noPlural},
    {"bn", oneUpToOne}, {"bo", noPlural}, {"bs", southSlavic}, {"ca", oneMillions},
    {"cs", czechSlovak}, {"csw", oneUpToOne}, {"doi", oneUpToOne}, {"dsb", slovenian},
    {"dz", noPlural}, {"es", oneMillions}, {"fa", oneUpToOne}, {"ff", oneUpToOne},
    {"fr", oneUpToOneMillions}, {"gu", oneUpToOne}, {"guw", oneUpToOne}, {"he", oneTwo},
    {"hi", oneUpToOne}, {"hnj", noPlural}, {"hr", southSlavic}, {"hsb", slovenian},
    {"hy", oneUpToOne}, {"id", noPlural}, {"ig", noPlural}, {"ii", noPlural},
    {"in", noPlural}, {"is", oneEndsInOne}, {"it", oneMillions}, {"iu", oneTwo},
    {"iw", oneTwo}, {"ja", noPlural}, {"jbo", noPlural}, {"jv", noPlural},
    {"jw", noPlural}, {"kab", oneUpToOne}, {"kde", noPlural}, {"kea", noPlural},
    {"km", noPlural}, {"kn", oneUpToOne}, {"ko", noPlural}, {"kok", oneUpToOne},
    {"kok_Latn", oneUpToOne}, {"lkt", noPlural}, {"lld", oneMillions}, {"ln", oneUpToOne},
    {"lo", noPlural}, {"lt", lithuanian}, {"lv", latvian}, {"mg", oneUpToOne},
    {"mk", oneEndsInOne}, {"mo", romanian}, {"ms", noPlural}, {"my", noPlural},
    {"naq", oneTwo}, {"nqo", noPlural}, {"nso", oneUpToOne}, {"osa", noPlural},
    {"pa", oneUpToOne}, {"pcm", oneUpToOne}, {"pl", polish}, {"prg", latvian},
    {"pt", oneUpToOneMillions}, {"pt_PT", oneMillions}, {"ro", romanian}, {"ru", eastSlavic},
    {"sah", noPlural}, {"sat", oneTwo}, {"scn", oneMillions}, {"se", oneTwo},
    {"ses", noPlural}, {"sg", noPlural}, {"sh", southSlavic}, {"si", oneUpToOne},
    {"sk", czechSlovak}, {"sl", slovenian}, {"sma", oneTwo}, {"smi", oneTwo},
    {"smj", oneTwo}, {"smn", oneTwo}, {"sms", oneTwo}, {"sr", southSlavic},
    {"su", noPlural}, {"th", noPlural}, {"ti", oneUpToOne}, {"to", noPlural},
    {"tpi", noPlural}, {"uk", eastSlavic}, {"vec", oneMillions}, {"vi", noPlural},
    {"wa", oneUpToOne}, {"wo", noPlural}, {"yo", noPlural}, {"yue", noPlural},
    {"zh", noPlural}, {"zu", oneUpToOne},
};

constexpr bool isSorted() {
    for (size_t i = 1; i < std::size(rules); ++i) {
        const char* a = rules[i - 1].identifier;
        const char* b = rules[i].identifier;
        while (*a && *a == *b) { ++a; ++b; }
        if (static_cast<unsigned char>(*a) >= static_cast<unsigned char>(*b)) return false;
    }
    return true;
}
static_assert(isSorted(), "plural rule table must be sorted by identifier");

#endif  // FOCUS_HAS_PLURAL_RULES

}  // namespace

PluralCategory detail::defaultPluralRule(uint64_t n) { return n == 1 ? P::One : P::Other; }

PluralRule detail::builtinPluralRule(const std::string& identifier) {
#if FOCUS_HAS_PLURAL_RULES
    auto it = std::lower_bound(std::begin(rules), std::end(rules), identifier,
        [](const RuleEntry& entry, const std::string& id) {
            return std::strcmp(entry.identifier, id.c_str()) < 0;
        });
    if (it != std::end(rules) && identifier == it->identifier) return it->rule;
#else
    (void)identifier;
#endif
    return nullptr;
}

}  // namespace focus
