#include "mainGame.h"
#include "graphics.h"
#include "vbe.h"
#include "memory.h"
#include "heap.h"
#include "string.h"
#include <arch/i686/io.h>
#include "time.h"

#include "gameAssets.h"

extern uint32_t* g_BackBuffer;

// Implementation of graphics_draw_rect to resolve the linker error
void graphics_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            draw_pixel(x + j, y + i, color);
        }
    }
}

static bool is_solid(GameState2D* state, int x, int y) {
    int tx = x / (TILE_SIZE * 100);
    int ty = y / (TILE_SIZE * 100);
    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) return true;
    return state->map[tx][ty] == TILE_SOLID;
}

// Gravity and Wall Slide bitchess woo
static void update_physics2d(GameState2D* state) {
    Player2D* p = &state->player;

    if (p->coyote_timer > 0) p->coyote_timer--;
    if (p->jump_buffer_timer > 0) p->jump_buffer_timer--;

    // 1. Gravity & Wall Slide
    int gravity = PHYSICS_GRAVITY;
    if ((p->on_wall_left || p->on_wall_right) && p->vy < 0) {
        gravity = PHYSICS_WALL_SLIDE;
    }
    p->vy -= gravity;
    if (p->vy < PHYSICS_TERMINAL_VELOCITY) p->vy = PHYSICS_TERMINAL_VELOCITY;

    // 2. Horizontal Friction
    p->vx = (p->vx * PHYSICS_FRICTION) / 100;

    // 3. Jump Logic (Including Wall Jumps)
    if (p->jump_buffer_timer > 0) {
        if (p->on_ground || p->coyote_timer > 0) {
            p->vy = PHYSICS_JUMP_FORCE;
            p->on_ground = false;
            p->coyote_timer = 0;
            p->jump_buffer_timer = 0;
        } else if (p->on_wall_left) {
            p->vy = PHYSICS_WALL_JUMP_UP; 
            p->vx = PHYSICS_WALL_JUMP_OUT;
            p->jump_buffer_timer = 0;
        } else if (p->on_wall_right) {
            p->vy = PHYSICS_WALL_JUMP_UP; 
            p->vx = -PHYSICS_WALL_JUMP_OUT;
            p->jump_buffer_timer = 0;
        }
    }

    // 4. Horizontal Movement and Collision Resolution
    p->x += p->vx;
    p->on_wall_left = is_solid(state, p->x - 100, p->y + 500) || is_solid(state, p->x - 100, p->y + p->height - 500);
    p->on_wall_right = is_solid(state, p->x + p->width + 100, p->y + 500) || is_solid(state, p->x + p->width + 100, p->y + p->height - 500);

    if (is_solid(state, p->x, p->y + 500) || is_solid(state, p->x, p->y + p->height - 500)) {
        // Snap to right side of tile
        p->x = ((p->x / (TILE_SIZE * 100)) + 1) * (TILE_SIZE * 100);
        p->vx = 0;
    } 
    else if (is_solid(state, p->x + p->width, p->y + 500) || is_solid(state, p->x + p->width, p->y + p->height - 500)) {
        // Snap to left side of tile
        p->x = ((p->x + p->width) / (TILE_SIZE * 100)) * (TILE_SIZE * 100) - p->width - 1;
        p->vx = 0;
    }

    // 5. Vertical Movement and Collision Resolution
    p->y += p->vy;
    p->on_ground = false;

    if (is_solid(state, p->x + 200, p->y) || is_solid(state, p->x + p->width - 200, p->y)) {
        // Snap to top of tile
        p->y = ((p->y / (TILE_SIZE * 100)) + 1) * (TILE_SIZE * 100);
        if (p->vy < 0) {
            p->on_ground = true;
            p->coyote_timer = 8;
        }
        p->vy = 0;
    }
    else if (is_solid(state, p->x + 200, p->y + p->height) || is_solid(state, p->x + p->width - 200, p->y + p->height)) {
        // Snap to bottom of tile (Head bonk)
        p->y = ((p->y + p->height) / (TILE_SIZE * 100)) * (TILE_SIZE * 100) - p->height - 1;
        p->vy = 0;
    }
}

static void render2d(GameState2D* state) {
    // Draw Map
    for (int x = 0; x < MAP_W; x++) {
        for (int y = 0; y < MAP_H; y++) {
            if (state->map[x][y] == TILE_SOLID) {
                graphics_draw_rect(x * TILE_SIZE, state->screen_h - (y + 1) * TILE_SIZE, 
                                 TILE_SIZE, TILE_SIZE, get_tile_color(TILE_SOLID));
            }
        }
    }

    // Draw Player
    graphics_draw_rect(state->player.x / 100, state->screen_h - (state->player.y / 100) - (state->player.height / 100),
                     state->player.width / 100, state->player.height / 100, 0x00FF0000);
}

void game2d_run() {
    if (!g_vbe_screen) return;

    GameState2D* state = (GameState2D*)malloc(sizeof(GameState2D));
    memset(state, 0, sizeof(GameState2D));

    state->screen_w = g_vbe_screen->width;
    state->screen_h = g_vbe_screen->height;
    state->back_buffer = g_BackBuffer;

    // Create a basic parkour level
    for (int x = 0; x < MAP_W; x++) state->map[x][0] = TILE_SOLID; // Floor
    for (int y = 0; y < 5; y++) state->map[5][y] = TILE_SOLID;    // A wall
    for (int y = 3; y < 8; y++) state->map[10][y] = TILE_SOLID;   // Another wall

    state->player.x = 2 * TILE_SIZE * 100;
    state->player.y = 5 * TILE_SIZE * 100;
    state->player.width = 20 * 100;
    state->player.height = 30 * 100;

    graphics_set_double_buffering(true);
    i686_outb(0x21, i686_inb(0x21) | 0x02); // Mask IRQ1

    while (1) {
        graphics_clear_buffer(0x00222222);
        
        update_physics2d(state);
        render2d(state);
        graphics_swap_buffer();
        sleep_ms(16); // Cap at ~60 FPS for consistent physics
        
        // Continuous Input Handling (Walking)
        if (state->key_states[0x1E]) state->player.vx -= PHYSICS_WALK_FORCE; // A
        if (state->key_states[0x20]) state->player.vx += PHYSICS_WALK_FORCE; // D

        // Input Handling
        while (i686_inb(0x64) & 0x01) {
            uint8_t sc = i686_inb(0x60);
            if (sc == 0x01) goto end_2d;

            if (sc & 0x80) {
                // Key Release (Break Code)
                state->key_states[sc & 0x7F] = false;
            } else {
                // Key Press (Make Code)
                state->key_states[sc] = true;
                if (sc == 0x39) state->player.jump_buffer_timer = 5; // Trigger jump buffer on press
            }
        }
    }

end_2d:
    i686_outb(0x21, i686_inb(0x21) & ~0x02);
    free(state);
    graphics_set_double_buffering(false);
}