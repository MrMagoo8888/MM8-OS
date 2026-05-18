#include "gameAssets.h"
#include "mainGame.h"

uint32_t get_tile_color(uint8_t tile_type) {
    switch (tile_type) {
        case TILE_SOLID: return 0x00555555; // Gray
        case TILE_GOAL:  return 0x00FFFF00; // Yellow
        case TILE_AIR:   return 0x00222222; // Background
        default:         return 0x00FF00FF; // Error Magenta
    }
}