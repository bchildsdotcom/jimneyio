#pragma once

#include "types.hpp"

enum JimneyMode {
    DARK=0,
    LIGHT=1
};

void drawJimny(PicoGraphics& graphics, Pens& pens, uint8_t offset_x, uint8_t offset_y, JimneyMode mode);
