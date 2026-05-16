#pragma once
#include "../../stdint.h"
#include "../../stdbool.h"

// World Dimensions
#define WORLD_W 10
#define WORLD_H 8
#define WORLD_D 10
#define BLOCK_SIZE 40

typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_BEDROCK,
    BLOCK_COUNT
} BlockType;

typedef struct {
    int x, y, z;        // Position (scaled by 100 for fixed-point)
    int vx, vy, vz;     // Velocity
    int angleY;         // Rotation
    bool on_ground;
} Player;

typedef struct {
    BlockType map[WORLD_W][WORLD_H][WORLD_D];
    Player player;
    uint32_t* z_buffer;
    uint32_t* back_buffer;
    int screen_w, screen_h;
} GameState;

// --- Core Functions ---

// Starts the game engine loop
void game_engine_run();

// Coordinate-based functions
void world_set_block(GameState* state, int x, int y, int z, BlockType type);
BlockType world_get_block(GameState* state, int x, int y, int z);