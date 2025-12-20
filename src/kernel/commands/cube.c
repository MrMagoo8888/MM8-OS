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

// Faces (indices into vertices array) - CW winding for backface culling
int faces[6][4] = {
    {0, 1, 2, 3}, // Front
    {1, 5, 6, 2}, // Right
    {5, 4, 7, 6}, // Back
    {4, 0, 3, 7}, // Left
    {4, 5, 1, 0}, // Top
    {3, 2, 6, 7}  // Bottom
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

// Helper to fill a triangle using scanlines
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    // Sort vertices by Y
    if (y1 > y2) { int t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }
    if (y1 > y3) { int t=x1; x1=x3; x3=t; t=y1; y1=y3; y3=t; }
    if (y2 > y3) { int t=x2; x2=x3; x3=t; t=y2; y2=y3; y3=t; }

    if (y1 == y3) return; // Zero height

    // Slopes (fixed point 16.16)
    int dx13 = 0, dx12 = 0, dx23 = 0;

    if (y3 > y1) dx13 = ((x3 - x1) << 16) / (y3 - y1);
    if (y2 > y1) dx12 = ((x2 - x1) << 16) / (y2 - y1);
    if (y3 > y2) dx23 = ((x3 - x2) << 16) / (y3 - y2);

    int wx1 = x1 << 16;
    int wx2 = x1 << 16;

    // Rasterize upper part (y1 to y2)
    for (int i = y1; i < y2; i++) {
        int a = wx1 >> 16;
        int b = wx2 >> 16;
        if (a > b) { int t=a; a=b; b=t; }
        draw_line(a, i, b + 1, i, color); // +1 to prevent cracks
        wx1 += dx13;
        wx2 += dx12;
    }

    // Rasterize lower part (y2 to y3)
    wx2 = x2 << 16; // Reset short edge walker to x2
    for (int i = y2; i < y3; i++) {
        int a = wx1 >> 16;
        int b = wx2 >> 16;
        if (a > b) { int t=a; a=b; b=t; }
        draw_line(a, i, b + 1, i, color);
        wx1 += dx13;
        wx2 += dx23;
    }
}

// Calculate 2D cross product (for winding order)
// Returns (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
int cross_product_2d(Point2D a, Point2D b, Point2D c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void wait_for_vsync() {
    // Wait for current retrace to end (if any)
    while (i686_inb(0x3DA) & 0x08);
    // Wait for next retrace to start
    while (!(i686_inb(0x3DA) & 0x08));
}

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

    int frame_counter = 0;

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
        Point3D rotated[8];

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

            // Store rotated vertices for lighting calculation
            rotated[i].x = x;
            rotated[i].y = y;
            rotated[i].z = z;

            // Project (Perspective)
            // distance = 200
            int distance = 200;
            int fov = 250;
            
            // Avoid division by zero
            if (distance + z == 0) z = 1;

            projected[i].x = (x * fov) / (distance + z) + half_w;
            projected[i].y = (y * fov) / (distance + z) + half_h;
        }

        // 4. Draw Faces (with backface culling)
        for (int i = 0; i < 6; i++) {
            Point2D p0 = projected[faces[i][0]];
            Point2D p1 = projected[faces[i][1]];
            Point2D p2 = projected[faces[i][2]];
            Point2D p3 = projected[faces[i][3]];

            // Check winding order (Cross product > 0 means facing camera)
            if (cross_product_2d(p0, p1, p2) > 0) {
                // Calculate lighting based on face normal (Z component)
                Point3D r0 = rotated[faces[i][0]];
                Point3D r1 = rotated[faces[i][1]];
                Point3D r2 = rotated[faces[i][2]];

                // Normal Z = (v1.x * v2.y) - (v1.y * v2.x)
                int ax = r1.x - r0.x;
                int ay = r1.y - r0.y;
                int bx = r2.x - r0.x;
                int by = r2.y - r0.y;
                
                int nz = ax * by - ay * bx;
                if (nz < 0) nz = -nz;

                // Map nz (0 to ~10000) to brightness (50 to 255)
                int brightness = 50 + (nz * 205) / 10000;
                if (brightness > 255) brightness = 255;
                
                uint32_t color = 0xFF000000 | (brightness << 16) | (brightness << 8) | brightness;

                fill_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
                fill_triangle(p0.x, p0.y, p2.x, p2.y, p3.x, p3.y, color);
            }
        }

        // Sync with Vertical Retrace to prevent flickering/tearing
        wait_for_vsync();

        // 5. Swap Buffer to Screen
        graphics_swap_buffer();

        // Update angles
        if (frame_counter++ % 5 == 0) {
            angleX = (angleX + 1) % 64;
            angleY = (angleY + 2) % 64;
            angleZ = (angleZ + 1) % 64;
        }

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