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

    int screen_w = g_vbe_screen->width;
    int screen_h = g_vbe_screen->height;

    // Trivial rejection Y
    if (y3 < 0 || y1 >= screen_h) return;

    // Slopes (fixed point 16.16)
    // Use long long to prevent overflow during shift
    int dx13 = 0, dx12 = 0, dx23 = 0;

    if (y3 > y1) dx13 = (int)(((long long)(x3 - x1) << 16) / (y3 - y1));
    if (y2 > y1) dx12 = (int)(((long long)(x2 - x1) << 16) / (y2 - y1));
    if (y3 > y2) dx23 = (int)(((long long)(x3 - x2) << 16) / (y3 - y2));

    long long wx1 = (long long)x1 << 16;
    long long wx2 = (long long)x1 << 16;

    // Rasterize upper part (y1 to y2)
    int y_start = y1;
    int y_end = y2;
    
    if (y_start < 0) y_start = 0;
    if (y_end > screen_h) y_end = screen_h;

    if (y_start < y_end) {
        // Advance walkers to y_start if we clipped the top
        if (y_start > y1) {
            int skip = y_start - y1;
            wx1 += (long long)dx13 * skip;
            wx2 += (long long)dx12 * skip;
        }

        for (int i = y_start; i < y_end; i++) {
            int a = (int)(wx1 >> 16);
            int b = (int)(wx2 >> 16);
            if (a > b) { int t=a; a=b; b=t; }
            if (a < 0) a = 0;
            if (b >= screen_w) b = screen_w - 1;
            if (a <= b) draw_line(a, i, b, i, color);
            wx1 += dx13;
            wx2 += dx12;
        }
        // If we clipped the bottom of this segment, advance wx1 to y2 for the next segment
        if (y_end < y2) {
            int remain = y2 - y_end;
            wx1 += (long long)dx13 * remain;
        }
    } else {
        // Segment completely skipped (off-screen), just advance wx1
        int dist = y2 - y1;
        wx1 += (long long)dx13 * dist;
    }

    // Rasterize lower part (y2 to y3)
    wx2 = (long long)x2 << 16; // Reset short edge walker to x2
    
    y_start = y2;
    y_end = y3;
    if (y_start < 0) y_start = 0;
    if (y_end > screen_h) y_end = screen_h;

    if (y_start < y_end) {
        if (y_start > y2) {
            int skip = y_start - y2;
            wx1 += (long long)dx13 * skip;
            wx2 += (long long)dx23 * skip;
        }

        for (int i = y_start; i < y_end; i++) {
            int a = (int)(wx1 >> 16);
            int b = (int)(wx2 >> 16);
            if (a > b) { int t=a; a=b; b=t; }
            if (a < 0) a = 0;
            if (b >= screen_w) b = screen_w - 1;
            if (a <= b) draw_line(a, i, b, i, color);
            wx1 += dx13;
            wx2 += dx23;
        }
    }
}

// Calculate 2D cross product (for winding order)
// Returns (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
int cross_product_2d(Point2D a, Point2D b, Point2D c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void wait_for_vsync() {
    // 1. Wait for the current retrace to finish (if we are in one)
    // This prevents exiting immediately if we catch the tail end of a VSync
    int timeout = 1000000;
    while (i686_inb(0x3DA) & 0x08) {
        if (--timeout == 0) break;
    }

    // 2. Wait for the next retrace to start
    timeout = 1000000;
    while (!(i686_inb(0x3DA) & 0x08)) {
        if (--timeout == 0) break;
    }
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

    int distance = 200;
    int fov = 250;

    // Loop until key press
    while (1) {
        
        // 1. Clear Buffer (Black)
        graphics_clear_buffer(0x00000000);

        // 2. Calculate Rotation
        // Divide by 4 to allow finer speed control (simulating fixed-point math)
        // This allows us to increment by 1 for slow speed (0.25 real steps), or 4 for normal speed
        int sinX = sin_table[(angleX / 8) % 64];
        int cosX = cos_table[(angleX / 8) % 64];
        int sinY = sin_table[(angleY / 8) % 64];
        int cosY = cos_table[(angleY / 8) % 64];
        int sinZ = sin_table[(angleZ / 8) % 64];
        int cosZ = cos_table[(angleZ / 8) % 64];

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

            // Near-plane culling:
            // If any vertex is too close to the camera (z + distance is small), 
            // the projection math explodes. Skip these faces.
            int too_close = 0;
            for (int k = 0; k < 4; k++) {
                if (rotated[faces[i][k]].z + distance < 50) { // 50 is a safety margin
                    too_close = 1;
                    break;
                }
            }
            if (too_close) continue;

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

                // Clamp coordinates to prevent integer overflow in fill_triangle
                // A range of +/- 4000 is safe for 16.16 fixed point math
                #define CLAMP(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
                int min_c = -4000;
                int max_c = 4000;

                int x0 = CLAMP(p0.x, min_c, max_c); int y0 = CLAMP(p0.y, min_c, max_c);
                int x1 = CLAMP(p1.x, min_c, max_c); int y1 = CLAMP(p1.y, min_c, max_c);
                int x2 = CLAMP(p2.x, min_c, max_c); int y2 = CLAMP(p2.y, min_c, max_c);
                int x3 = CLAMP(p3.x, min_c, max_c); int y3 = CLAMP(p3.y, min_c, max_c);

                fill_triangle(x0, y0, x1, y1, x2, y2, color);
                fill_triangle(x0, y0, x2, y2, x3, y3, color);
            }
        }

        // Sync with monitor refresh to prevent tearing
        // Disable interrupts to prevent the copy from being paused by the timer
        __asm__ volatile ("cli");
        wait_for_vsync();

        // 5. Swap Buffer to Screen
        graphics_swap_buffer();
        __asm__ volatile ("sti");

        // Update angles
        // Change these values to adjust speed.
        // +1 is now slow (quarter speed). +4 would be the original speed.
        angleX = (angleX + 1) % 256; // Range is now 0-255 (64 * 4)
        angleY = (angleY + 2) % 256;
        angleZ = (angleZ + 1) % 256;

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