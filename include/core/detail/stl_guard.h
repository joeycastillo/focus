/*
 * Focus internal — not part of the public API. Include only from Focus headers.
 *
 * Arduino.h defines round(), abs(), min(), and max() as function-like macros.
 * Once they are live, any standard-library header parsed afterward that uses
 * those names as identifiers is corrupted: on libstdc++ that includes
 * <chrono> (std::chrono::round/abs/floor/ceil) and <algorithm> (std::min/max)
 * directly, and on every implementation the C-library headers <cmath>,
 * <cstdlib>, and <math.h> that the containers pull in transitively.
 *
 * This header parses Focus's entire standard-library footprint exactly once
 * with those macros neutralized. Because each of those headers then defines
 * its own include guard, every later `#include` of them anywhere in the
 * translation unit is a no-op that reuses the clean copy — regardless of
 * whether Arduino.h defined the macros before or after this point.
 *
 * Requirement: this must be the FIRST include in any Focus header that a
 * translation unit may reach before it has parsed the standard library. The
 * macros are pushed and popped, so Arduino code downstream keeps its round()/
 * abs()/min()/max(). When the macros are not defined (host/ESP-IDF builds)
 * the push/undef/pop is a harmless no-op and this simply pre-includes the STL.
 */
#pragma once

#pragma push_macro("round")
#pragma push_macro("abs")
#pragma push_macro("min")
#pragma push_macro("max")
#undef round
#undef abs
#undef min
#undef max

// C-library headers carrying the corrupted names, parsed clean first.
#include <cstdlib>
#include <cmath>
#include <ctime>

// STL headers whose own bodies use the names (libstdc++), plus the rest of
// Focus's footprint so a single warm-up covers every later include site.
#include <chrono>
#include <algorithm>
#include <array>
#include <any>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#pragma pop_macro("max")
#pragma pop_macro("min")
#pragma pop_macro("abs")
#pragma pop_macro("round")
