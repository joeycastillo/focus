#pragma once

#include <stdint.h>

typedef struct {
    uint8_t control: 1;     /// <- is a control character
    uint8_t whitespace: 1;  /// <- character is whitespace
    uint8_t linebreak: 1;   /// <- a line break opportunity exists when this character appears in a run
    uint8_t nsm: 1;         /// <- is a nonspacing mark
    uint8_t rtl: 1;         /// <- has a strong RTL affinity
    uint8_t ltr: 1;         /// <- has a strong LTR affinity
    uint8_t mirrored: 1;    /// <- draws mirrored in RTL text runs
    uint8_t hasMirror: 1;   /// <- a mapping exists to this character's mirror image
} unicode_info_t;
