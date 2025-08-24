#pragma once

#include <stdint.h>
#include "utf8_decode.hpp"

typedef union {
    struct {
        uint8_t controlchar: 1; /// <- is a control character
        uint8_t whitespace: 1;  /// <- character is whitespace
        uint8_t linebreak: 1;   /// <- a line break opportunity exists when this character appears in a run
        uint8_t nsm: 1;         /// <- is a nonspacing mark
        uint8_t rtl: 1;         /// <- has a strong RTL affinity
        uint8_t ltr: 1;         /// <- has a strong LTR affinity
        uint8_t mirrored: 1;    /// <- draws mirrored in RTL text runs
        uint8_t mapped: 1;      /// <- a mapping exists to this character's mirror image
    } is;
    uint8_t packed;
} unicode_info_t;

unicode_info_t getTraitsForCodepoint(UNICODE_CODEPOINT codepoint);
