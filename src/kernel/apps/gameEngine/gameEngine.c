#include "gameEngine.h"
#include "graphics.h"
#include "vbe.h"
#include "stdio.h"
#include "memory.h"
#include "heap.h"
#include "string.h"
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h>

extern uint32_t* g_BackBuffer;

// --- Math & Tables (Reused from cube.c) ---

static int sin_table[64] = {
    0, 12, 25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 118, 122, 125, 127,
    128, 127, 125, 122, 118, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -118, -122, -125, -127,
    -128, -127, -125, -122, -118, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12
};

static int cos_table[64] = {
    128, 127, 125, 122, 118, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -118, -122, -125, -127,
    -128, -127, -125, -122, -118, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12,
    0, 12, 25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 118, 122, 125, 127
};

static uint32_t block_colors[BLOCK_COUNT] = {
    0x00000000, // Air
    0x0022AA22, // Grass
    0x00774422, // Dirt
    0x00888888, // Stone
    0x00222222  // Bedrock
};

typedef struct { int x, y, z; } Point3D;
typedef struct { int x, y, depth; } Point2D;

static Point3D cube_verts[8] = {
    {0, 0, 0}, {BLOCK_SIZE, 0, 0}, {BLOCK_SIZE, BLOCK_SIZE, 0}, {0, BLOCK_SIZE, 0},
    {0, 0, BLOCK_SIZE}, {BLOCK_SIZE, 0, BLOCK_SIZE}, {BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE}, {0, BLOCK_SIZE, BLOCK_SIZE}
};

static int cube_faces[6][4] = {
    {0, 1, 2, 3}, {1, 5, 6, 2}, {5, 4, 7, 6}, {4, 0, 3, 7}, {4, 5, 1, 0}, {3, 2, 6, 7}
};

// --- Helpers ---

void world_set_block(GameState* state, int x, int y, int z, BlockType type) {
    if (x >= 0 && x < WORLD_W && y >= 0 && y < WORLD_H && z >= 0 && z < WORLD_D)
        state->map[x][y][z] = type;
}

BlockType world_get_block(GameState* state, int x, int y, int z) {
    if (x >= 0 && x < WORLD_W && y >= 0 && y < WORLD_H && z >= 0 && z < WORLD_D)
        return state->map[x][y][z];
    return BLOCK_AIR;
}

static void swap_int(int* a, int* b) {
    int temp = *a; *a = *b; *b = temp;
}

static void draw_triangle(GameState* state, Point2D p0, Point2D p1, Point2D p2, uint32_t color) {
    if (p0.y > p1.y) { Point2D t = p0; p0 = p1; p1 = t; }
    if (p0.y > p2.y) { Point2D t = p0; p0 = p2; p2 = t; }
    if (p1.y > p2.y) { Point2D t = p1; p1 = p2; p2 = t; }

    int total_height = p2.y - p0.y;
    if (total_height == 0) return;

    for (int i = 0; i < total_height; i++) {
        int y = p0.y + i;
        if (y < 0 || y >= state->screen_h) continue;

        bool second_half = i > p1.y - p0.y || p1.y == p0.y;
        int segment_height = second_half ? p2.y - p1.y : p1.y - p0.y;
        if (segment_height == 0) continue;

        int alpha = i * 1024 / total_height;
        int beta  = (i - (second_half ? p1.y - p0.y : 0)) * 1024 / segment_height;

        int Ax = p0.x + (p2.x - p0.x) * alpha / 1024;
        int Az = p0.depth + (p2.depth - p0.depth) * alpha / 1024;
        int Bx = second_half ? p1.x + (p2.x - p1.x) * beta / 1024 : p0.x + (p1.x - p0.x) * beta / 1024;
        int Bz = second_half ? p1.depth + (p2.depth - p1.depth) * beta / 1024 : p0.depth + (p1.depth - p0.depth) * beta / 1024;

        if (Ax > Bx) { swap_int(&Ax, &Bx); swap_int(&Az, &Bz); }

        // Horizontal Clipping
        int left = Ax, right = Bx;
        if (left < 0) left = 0;
        if (right >= state->screen_w) right = state->screen_w - 1;
        if (left > right) continue;

        int z_step = (Bx - Ax) > 0 ? (Bz - Az) * 1024 / (Bx - Ax) : 0;
        int curr_z_scaled = Az * 1024 + (left - Ax) * z_step;

        int row_offset = y * state->screen_w;
        uint32_t* row_ptr = &state->back_buffer[row_offset];
        uint32_t* z_ptr = &state->z_buffer[row_offset];

        for (int j = left; j <= right; j++) {
            // Use bit-shifting instead of division for speed
            int curr_z = curr_z_scaled >> 10;
            
            // Direct memory access: no draw_pixel() overhead
            if (curr_z > 0 && (uint32_t)curr_z < z_ptr[j]) {
                z_ptr[j] = (uint32_t)curr_z;
                row_ptr[j] = color;
            }
            curr_z_scaled += z_step;
        }
    }
}

// --- Physics & Logic ---

static bool check_collision(GameState* state, int x, int y, int z) {
    // Convert world space (scaled by 100) to block indices
    int bx = x / (BLOCK_SIZE * 100);
    int by = y / (BLOCK_SIZE * 100);
    int bz = z / (BLOCK_SIZE * 100);
    return world_get_block(state, bx, by, bz) != BLOCK_AIR;
}

static void update_physics(GameState* state) {
    Player* p = &state->player;
    
    // Gravity
    p->vy -= 50; 
    if (p->vy < -1000) p->vy = -1000;

    // Apply Velocity and Collision (Simple point-based for demo)
    int next_x = p->x + p->vx;
    int next_y = p->y + p->vy;
    int next_z = p->z + p->vz;

    if (!check_collision(state, next_x, p->y, p->z)) p->x = next_x; else p->vx = 0;
    
    if (!check_collision(state, p->x, next_y, p->z)) {
        p->y = next_y;
        p->on_ground = false;
    } else {
        if (p->vy < 0) p->on_ground = true;
        p->vy = 0;
    }

    if (!check_collision(state, p->x, p->y, next_z)) p->z = next_z; else p->vz = 0;

    // Friction
    p->vx = (p->vx * 70) / 100;
    p->vz = (p->vz * 70) / 100;
}

// --- Rendering ---

static void render_world(GameState* state) {
    int half_w = state->screen_w / 2;
    int half_h = state->screen_h / 2;
    int sinY = sin_table[state->player.angleY % 64];
    int cosY = cos_table[state->player.angleY % 64];

    // Neighbor offsets for 6 faces: Back, Right, Front, Left, Bottom, Top
    static int dx_offsets[] = { 0,  1,  0, -1,  0,  0};
    static int dy_offsets[] = { 0,  0,  0,  0, -1,  1};
    static int dz_offsets[] = {-1,  0,  1,  0,  0,  0};

    for (int bx = 0; bx < WORLD_W; bx++) {
        for (int by = 0; by < WORLD_H; by++) {
            for (int bz = 0; bz < WORLD_D; bz++) {
                BlockType type = state->map[bx][by][bz];
                if (type == BLOCK_AIR) continue;

                Point2D projected[8];
                int visible_count = 0;

                // Transform all 8 vertices of the block
                for (int i = 0; i < 8; i++) {
                    int wx = bx * BLOCK_SIZE + cube_verts[i].x - (state->player.x / 100);
                    int wy = by * BLOCK_SIZE + cube_verts[i].y - (state->player.y / 100);
                    int wz = bz * BLOCK_SIZE + cube_verts[i].z - (state->player.z / 100);

                    int rx = (wx * cosY + wz * sinY) / 128;
                    int rz = (-wx * sinY + wz * cosY) / 128;

                    int depth = rz;
                    if (depth < 5) depth = 5; 
                    if (rz > 0) visible_count++;

                    projected[i].x = (rx * 256) / depth + half_w;
                    projected[i].y = half_h - ((wy - 30) * 256) / depth; // Eye-level offset (+30)
                    projected[i].depth = depth;
                }

                // If all vertices are behind the camera, skip the block
                if (visible_count == 0) continue;

                uint32_t color = block_colors[type];
                for (int f = 0; f < 6; f++) {
                    // --- Occlusion Check ---
                    // Only draw the face if the neighbor is AIR or outside the world
                    int nx = bx + dx_offsets[f];
                    int ny = by + dy_offsets[f];
                    int nz = bz + dz_offsets[f];

                    if (nx >= 0 && nx < WORLD_W && ny >= 0 && ny < WORLD_H && nz >= 0 && nz < WORLD_D) {
                        if (state->map[nx][ny][nz] != BLOCK_AIR) {
                            continue; // Skip rendering this face
                        }
                    }

                    Point2D p0 = projected[cube_faces[f][0]];
                    Point2D p1 = projected[cube_faces[f][1]];
                    Point2D p2 = projected[cube_faces[f][2]];
                    Point2D p3 = projected[cube_faces[f][3]];

                    // Back-face culling (Standard cross product area check)
                    int area = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
                    if (area < 0) {
                        // Shade the colors based on face direction for a 3D effect
                        uint32_t face_color = color;
                        if (f == 1 || f == 3) face_color = (color >> 1) & 0x7F7F7F7F; // Side shading
                        else if (f == 4 || f == 0) face_color = (color >> 2) & 0x3F3F3F3F; // Top/Bottom shading
                        
                        draw_triangle(state, p0, p1, p2, face_color);
                        draw_triangle(state, p0, p2, p3, face_color);
                    }
                }
            }
        }
    }
}

void game_engine_run() {
    if (!g_vbe_screen) return;

    GameState* state = (GameState*)malloc(sizeof(GameState));
    state->screen_w = g_vbe_screen->width;
    state->screen_h = g_vbe_screen->height;
    state->z_buffer = (uint32_t*)malloc(state->screen_w * state->screen_h * sizeof(uint32_t));
    state->back_buffer = g_BackBuffer;

    // Init World: A solid 5x5 platform
    memset(state->map, 0, sizeof(state->map));
    for (int x = 2; x < 7; x++) {
        for (int z = 2; z < 7; z++) {
            world_set_block(state, x, 0, z, BLOCK_STONE);
        }
    }

    // Init Player: Standing in the middle of the platform
    state->player.x = (4 * BLOCK_SIZE * 100) + 2000;
    state->player.y = 120 * 100; // Start slightly in the air to land on the platform
    state->player.z = (4 * BLOCK_SIZE * 100) + 2000;
    state->player.angleY = 32; // Look forward
    state->player.vx = state->player.vy = state->player.vz = 0;

    graphics_set_double_buffering(true);
    i686_outb(0x21, i686_inb(0x21) | 0x02); // Mask IRQ1

    while (1) {
        graphics_clear_buffer(0x0088CCFF); // Sky Blue
        memset(state->z_buffer, 0xFF, state->screen_w * state->screen_h * sizeof(uint32_t));

        update_physics(state);
        render_world(state);
        graphics_swap_buffer();

        // Input Handling
        if (i686_inb(0x64) & 0x01) {
            uint8_t sc = i686_inb(0x60);
            if (sc == 0x01) break; // ESC

            int speed = 200;
            int sinY = sin_table[state->player.angleY % 64];
            int cosY = cos_table[state->player.angleY % 64];

            switch (sc) {
                case 0x11: // W
                    state->player.vx += (sinY * speed) / 128;
                    state->player.vz += (cosY * speed) / 128;
                    break;
                case 0x1F: // S
                    state->player.vx -= (sinY * speed) / 128;
                    state->player.vz -= (cosY * speed) / 128;
                    break;
                case 0x1E: // A (Strafe Left)
                    state->player.vx -= (cosY * speed) / 128;
                    state->player.vz += (sinY * speed) / 128;
                    break;
                case 0x20: // D (Strafe Right)
                    state->player.vx += (cosY * speed) / 128;
                    state->player.vz -= (sinY * speed) / 128;
                    break;
                case 0x4B: // Left Arrow (Turn Left)
                    state->player.angleY = (state->player.angleY + 62) % 64;
                    break;
                case 0x4D: // Right Arrow (Turn Right)
                    state->player.angleY = (state->player.angleY + 2) % 64;
                    break;
                case 0x39: // Space
                    if (state->player.on_ground) state->player.vy = 800;
                    break;
            }
        }
    }

    i686_outb(0x21, i686_inb(0x21) & ~0x02);
    free(state->z_buffer);
    free(state);
    graphics_set_double_buffering(false);
}