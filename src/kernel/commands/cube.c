#include "cube.h"
#include "../graphics.h"
#include "../vbe.h"
#include "../stdio.h"
#include "../memory.h"
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h>
#include "../heap.h"

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

// Faces (indices into vertices array)
// Defined in Clockwise order for culling
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
    -9, -6, -3, 0 // Fill remaining 4 elements to smooth transition
};

int cos_table[64] = {
    128, 127, 124, 119, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12, 0,
    -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -119, -124, -127, -128, -127,
    -124, -119, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12, 0, 12,
    25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 119, 124, 127, 128, 127, 124,
    125, 126, 127, 128 // Fill remaining 4 elements to smooth transition
};

static uint32_t face_colors[6] = {
    0x00FF0000, // Front - Red
    0x0000FF00, // Right - Green
    0x000000FF, // Back - Blue
    0x00FFFF00, // Left - Yellow
    0x0000FFFF, // Top - Cyan
    0x00FF00FF  // Bottom - Magenta
};

static void swap_int(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

static uint32_t apply_light(uint32_t color, int intensity) {
    if (intensity < 0) intensity = 0;
    if (intensity > 255) intensity = 255;
    
    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;
    
    r = (r * intensity) / 255;
    g = (g * intensity) / 255;
    b = (b * intensity) / 255;
    
    return (r << 16) | (g << 8) | b;
}

static void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    // Sort vertices by y
    if (y0 > y1) { swap_int(&x0, &x1); swap_int(&y0, &y1); }
    if (y0 > y2) { swap_int(&x0, &x2); swap_int(&y0, &y2); }
    if (y1 > y2) { swap_int(&x1, &x2); swap_int(&y1, &y2); }

    int total_height = y2 - y0;
    if (total_height == 0) return;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;
        if (segment_height == 0) continue;

        int A_x = x0 + (x2 - x0) * i / total_height;
        
        int offset = second_half ? y1 - y0 : 0;
        int B_x = second_half ? 
                  x1 + (x2 - x1) * (i - offset) / segment_height : 
                  x0 + (x1 - x0) * (i - offset) / segment_height;
        
        if (A_x > B_x) { swap_int(&A_x, &B_x); }
        
        for (int j = A_x; j <= B_x; j++) {
            draw_pixel(j, y0 + i, color);
        }
    }
}

void cube_test() {
    if (!g_vbe_screen) {
        printf("Error: Graphics not initialized.\n");
        getch();
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

    // Dynamic Buffer: Allocate memory for projected points
    Point2D* projected = (Point2D*)malloc(8 * sizeof(Point2D));
    if (!projected) {
        printf("Error: Failed to allocate memory for projection.\n");
        getch();
        return;
    }

    // Mask IRQ 1 (Keyboard) to prevent the OS ISR from stealing the keypress
    i686_outb(0x21, i686_inb(0x21) | 0x02);

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

            int px = (x * fov) / (distance + z) + half_w;
            int py = (y * fov) / (distance + z) + half_h;

            // Clamp coordinates to screen bounds to prevent memory corruption
            if (px < 0) px = 0;
            if (px >= screen_w) px = screen_w - 1;
            if (py < 0) py = 0;
            if (py >= screen_h) py = screen_h - 1;

            projected[i].x = px;
            projected[i].y = py;
        }

        // 4. Draw Faces (with Back-face Culling)
        for (int f = 0; f < 6; f++) {
            Point2D p0 = projected[faces[f][0]];
            Point2D p1 = projected[faces[f][1]];
            Point2D p2 = projected[faces[f][2]];
            Point2D p3 = projected[faces[f][3]];

            // Calculate signed area (2D Cross Product) to determine visibility
            // If area > 0, the face is facing the camera (Clockwise winding)
            int area = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);

            if (area > 0) {
                // Use area as a proxy for lighting intensity (Lambertian / Diffuse)
                int intensity = area * 200 / 15000 + 55; // Base ambient + directional
                uint32_t color = apply_light(face_colors[f], intensity);

                draw_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
                draw_triangle(p0.x, p0.y, p2.x, p2.y, p3.x, p3.y, color);
            }
        }

        // 5. Swap Buffer to Screen
        graphics_swap_buffer();

        // Update angles
        angleX = (angleX + 1) % 64;
        angleY = (angleY + 2) % 64;
        angleZ = (angleZ + 1) % 64;

        // Simple delay
        for (volatile int d = 0; d < 1000000; d++);

        // Check for keypress (Polling directly, IRQ 1 is masked)
        if (i686_inb(0x64) & 0x01) {
            uint8_t scancode = i686_inb(0x60);
            if ((scancode & 0x80) == 0) { // Only break on Key Press (Make code), ignore Key Release
                break;
            }
        }
    }

    // Restore IRQ 1 (Unmask Keyboard)
    i686_outb(0x21, i686_inb(0x21) & ~0x02);

    // Free dynamic memory
    free(projected);

    // Cleanup: Disable double buffering (returns to direct drawing)
    graphics_set_double_buffering(false);
    clrscr(); // Clear screen back to text mode look
}