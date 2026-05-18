#pragma once
#include "stdint.h"
#include "stdbool.h"

#define MAP_W 25
#define MAP_H 15
#define TILE_SIZE 32

// --- Tuning Variables ---
#define PHYSICS_GRAVITY          45
#define PHYSICS_WALL_SLIDE       15
#define PHYSICS_FRICTION         85  // Percentage (0-100)
#define PHYSICS_WALK_FORCE       200
#define PHYSICS_JUMP_FORCE       1100
#define PHYSICS_WALL_JUMP_UP     950
#define PHYSICS_WALL_JUMP_OUT    850
#define PHYSICS_TERMINAL_VELOCITY -1600

typedef struct {
    int x, y;       // Position (scaled by 100)
    int vx, vy;     // Velocity
    bool on_ground;
    bool on_wall_left;
    bool on_wall_right;
    int width, height;
    int coyote_timer;       // Grace period after leaving ground
    int jump_buffer_timer;  // Grace period for early jump press
} Player2D;

typedef struct {
    uint8_t map[MAP_W][MAP_H];
    Player2D player;
    int screen_w, screen_h;
    uint32_t* back_buffer;
    bool key_states[256];
} GameState2D;

// Block types
#define TILE_AIR 0
#define TILE_SOLID 1
#define TILE_GOAL 2

// External Assets
extern uint32_t get_tile_color(uint8_t tile_type);

void game2d_run();
