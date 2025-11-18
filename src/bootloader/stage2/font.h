#pragma once
#include "stdint.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

// A simple 8x16 font (Code Page 437)
extern uint8_t default_font[256][FONT_HEIGHT];