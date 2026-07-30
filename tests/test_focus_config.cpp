#include "test_harness.hpp"
#include "focus_config.h"

// On the hosted test build no platform macro is defined, so the flag must default to 1.
static_assert(FOCUS_HAS_FILESYSTEM == 1,
              "hosted build should default to FOCUS_HAS_FILESYSTEM=1");

TEST(focus_config_defaults_to_filesystem_present) {
    ASSERT_EQ(FOCUS_HAS_FILESYSTEM, 1);
}
