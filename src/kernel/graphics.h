#pragma once

#include <stdint.h>

/**
 * @brief Draws a single "pixel" (character) at a given coordinate.
 */
void draw_pixel(int x, int y, char c, uint8_t color);

/**
 * @brief Draws a filled rectangle.
 */
void draw_rect(int x, int y, int width, int height, char c, uint8_t color);

/**
 * @brief Draws a filled circle.
 */
void draw_circle(int centerX, int centerY, int radius, char c, uint8_t color);