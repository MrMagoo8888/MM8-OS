#include "cube.h"
#include "../graphics.h"
#include "../vbe.h"
#include "../stdio.h"
#include "../memory.h"
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h>

// --- 3D Math Helpers ---

typedef struct {
    int x, y, z;
} Point3D;

typedef struct {
    int x, y;
} Point2D;

// Cube vertices (centered at 0,0,0)
// Size 50
#define CUBE_SIZE 50
Point3D vertices[8] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE}
};

// Edges connecting the vertices (indices into vertices array)
int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // Front face
    {4,5}, {5,6}, {6,7}, {7,4}, // Back face
    {0,4}, {1,5}, {2,6}, {3,7}  // Connecting lines
};

// Simple Sin/Cos lookup table (0 to 360 degrees approx)
// Scaled by 128 for integer math
// 64 steps for a full circle
int sin_table[64] = {
    0, 12, 25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 119, 124, 127, 128,
    127, 124, 119, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12,
    -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -119, -124, -127, -128,
    -127, -124, -119, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12,
    0, 12, 25, 37 // Overlap slightly
};

int cos_table[64] = {
    128, 127, 124, 119, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12, 0,
    -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -119, -124, -127, -128, -127,
    -124, -119, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12, 0, 12,
    25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 119, 124, 127, 128, 127, 124
};

void cube_test() {
    if (!g_vbe_screen) {
        printf("Error: Graphics not initialized.\n");
        return;
    }

    printf("Starting Cube Test... Press any key to exit.\n");

    // Enable double buffering
    graphics_set_double_buffering(true);

    int angleX = 0;
    int angleY = 0;
    int angleZ = 0;

    int screen_w = g_vbe_screen->width;
    int screen_h = g_vbe_screen->height;
    int half_w = screen_w / 2;
    int half_h = screen_h / 2;

    // Disable interrupts to prevent the ISR from consuming the keypress
    __asm__ volatile ("cli");

    // Loop until key press
    while (1) {
        
        // 1. Clear Buffer (Black)
        graphics_clear_buffer(0x00000000);

        // 2. Calculate Rotation
        int sinX = sin_table[angleX % 64];
        int cosX = cos_table[angleX % 64];
        int sinY = sin_table[angleY % 64];
        int cosY = cos_table[angleY % 64];
        int sinZ = sin_table[angleZ % 64];
        int cosZ = cos_table[angleZ % 64];

        Point2D projected[8];

        // 3. Transform Vertices
        for (int i = 0; i < 8; i++) {
            int x = vertices[i].x;
            int y = vertices[i].y;
            int z = vertices[i].z;

            // Rotate X
            int y1 = (y * cosX - z * sinX) / 128;
            int z1 = (y * sinX + z * cosX) / 128;
            y = y1; z = z1;

            // Rotate Y
            int x1 = (x * cosY + z * sinY) / 128;
            z1 = (-x * sinY + z * cosY) / 128;
            x = x1; z = z1;

            // Rotate Z
            x1 = (x * cosZ - y * sinZ) / 128;
            y1 = (x * sinZ + y * cosZ) / 128;
            x = x1; y = y1;

            // Project (Perspective)
            // distance = 200
            int distance = 200;
            int fov = 250;
            
            // Avoid division by zero
            if (distance + z == 0) z = 1;

            projected[i].x = (x * fov) / (distance + z) + half_w;
            projected[i].y = (y * fov) / (distance + z) + half_h;
        }

        // 4. Draw Edges
        for (int i = 0; i < 12; i++) {
            int p1 = edges[i][0];
            int p2 = edges[i][1];
            draw_line(projected[p1].x, projected[p1].y, 
                      projected[p2].x, projected[p2].y, 
                      0x0000FF00); // Green
        }

        // 5. Swap Buffer to Screen
        graphics_swap_buffer();

        // Update angles
        angleX = (angleX + 1) % 64;
        angleY = (angleY + 2) % 64;
        angleZ = (angleZ + 1) % 64;

        // Simple delay
        for (volatile int d = 0; d < 1000000; d++);

        // Check for keypress (Status Register bit 0 set)
        if (i686_inb(0x64) & 0x01) {
            uint8_t scancode = i686_inb(0x60);
            if ((scancode & 0x80) == 0) { // Only break on Key Press (Make code), ignore Key Release
                break;
            }
        }
    }

    // Re-enable interrupts
    __asm__ volatile ("sti");

    // Cleanup: Disable double buffering (returns to direct drawing)
    graphics_set_double_buffering(false);
    clrscr(); // Clear screen back to text mode look
}