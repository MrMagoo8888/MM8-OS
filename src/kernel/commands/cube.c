#include "cube.h"
#include "../graphics.h"
#include "../vbe.h"
#include "../stdio.h"
#include "../memory.h"
#include <arch/i686/io.h>
#include <arch/i686/keyboard.h>
#include "../heap.h"
#include "../font.h"
#include "../string.h"

// --- 3D Math Helpers ---

typedef struct {
    int x, y, z;
} Point3D;

typedef struct {
    int x, y, depth;
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
    0, 12, 25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 118, 122, 125, 127,
    128, 127, 125, 122, 118, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -118, -122, -125, -127,
    -128, -127, -125, -122, -118, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12
};

int cos_table[64] = {
    128, 127, 125, 122, 118, 113, 107, 99, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -99, -107, -113, -118, -122, -125, -127,
    -128, -127, -125, -122, -118, -113, -107, -99, -90, -81, -71, -60, -49, -37, -25, -12,
    0, 12, 25, 37, 49, 60, 71, 81, 90, 99, 107, 113, 118, 122, 125, 127
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

static void draw_triangle(int x0, int y0, int z0, int x1, int y1, int z1, int x2, int y2, int z2, uint32_t color, uint32_t* z_buffer, int screen_w, int screen_h) {
    // Sort vertices by y
    if (y0 > y1) { swap_int(&x0, &x1); swap_int(&y0, &y1); swap_int(&z0, &z1); }
    if (y0 > y2) { swap_int(&x0, &x2); swap_int(&y0, &y2); swap_int(&z0, &z2); }
    if (y1 > y2) { swap_int(&x1, &x2); swap_int(&y1, &y2); swap_int(&z1, &z2); }

    int total_height = y2 - y0;
    if (total_height == 0) return;

    for (int i = 0; i < total_height; i++) {
        int y = y0 + i;
        if (y < 0 || y >= screen_h) continue;

        bool second_half = i > y1 - y0 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;
        if (segment_height == 0) continue;

        int alpha = i * 1024 / total_height;
        int beta  = (i - (second_half ? y1 - y0 : 0)) * 1024 / segment_height;

        int A_x = x0 + (x2 - x0) * alpha / 1024;
        int A_z = z0 + (z2 - z0) * alpha / 1024;
        
        int B_x = second_half ? x1 + (x2 - x1) * beta / 1024 : x0 + (x1 - x0) * beta / 1024;
        int B_z = second_half ? z1 + (z2 - z1) * beta / 1024 : z0 + (z1 - z0) * beta / 1024;
        
        if (A_x > B_x) { swap_int(&A_x, &B_x); swap_int(&A_z, &B_z); }
        
        int z_step = (B_x - A_x) > 0 ? (B_z - A_z) * 1024 / (B_x - A_x) : 0;
        int current_z_scaled = A_z * 1024;

        int row_offset = y * screen_w;
        for (int j = A_x; j <= B_x; j++) {
            if (j >= 0 && j < screen_w) {
                int current_z = current_z_scaled / 1024;
                int idx = row_offset + j;
                if ((uint32_t)current_z < z_buffer[idx]) {
                    z_buffer[idx] = (uint32_t)current_z;
                    draw_pixel(j, y, color);
                }
            }
            current_z_scaled += z_step;
        }
    }
}

static void wait_for_vsync() {
    // Wait for the current retrace to finish (if we are in one)
    while (i686_inb(0x3DA) & 8);
    
    // Wait for the next retrace to start
    while (!(i686_inb(0x3DA) & 8));
}

// --- Text Drawing Helpers ---

// Make g_ticks from main.c available
extern volatile uint32_t g_ticks;

// Helper to draw a character with a transparent background
static void draw_char_transparent(int x, int y, char c, uint32_t color) {
    if (!g_vbe_screen) return;

    // Get font data (offset by 32 because our font starts at space)
    if (c < 32 || c > 127) c = '?'; // Use a placeholder for non-printable chars
    const uint8_t* glyph = font8x8_basic[c - 32];

    // Draw 8x8 pixels
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Check if the bit is set in the font bitmap
            if ((glyph[row] >> (7 - col)) & 1) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

// Helper to draw a string
static void draw_string(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        draw_char_transparent(x, y, *str, color);
        x += 8; // Move to next character position
        str++;
    }
}

// --- Integer to String Conversion ---

static void reverse(char s[]) {
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

static void itoa(int n, char s[]) {
    int i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    s[i] = '\0';
    reverse(s);
}

static void render_cube(int angleX, int angleY, int angleZ, 
                        int offsetX, int offsetY, int offsetZ, int size,
                        int camX, int camY, int camZ,
                        Point2D* projected, uint32_t* z_buffer, 
                        int screen_w, int screen_h)
{
    int half_w = screen_w / 2;
    int half_h = screen_h / 2;
    
    // 1. Calculate Rotation
    int sinX = sin_table[angleX % 64];
    int cosX = cos_table[angleX % 64];
    int sinY = sin_table[angleY % 64];
    int cosY = cos_table[angleY % 64];
    int sinZ = sin_table[angleZ % 64];
    int cosZ = cos_table[angleZ % 64];

    // 2. Transform Vertices
    for (int i = 0; i < 8; i++) {
        // Scale original vertex
        int x = (vertices[i].x * size) / CUBE_SIZE;
        int y = (vertices[i].y * size) / CUBE_SIZE;
        int z = (vertices[i].z * size) / CUBE_SIZE;

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

        // Translate
        x += offsetX; // to world space
        y += offsetY;
        z += offsetZ;

        // Transform to camera space
        x -= camX;
        y -= camY;
        z -= camZ;

        // Project (Perspective)
        int distance_to_plane = 200;
        int fov = 250;
        int depth_val = distance_to_plane + z;
        if (depth_val == 0) depth_val = 1; // Avoid division by zero

        projected[i].x = (x * fov) / depth_val + half_w;
        projected[i].y = (y * fov) / depth_val + half_h;
        projected[i].depth = depth_val;
    }

    // 3. Draw Faces (with Back-face Culling)
    for (int f = 0; f < 6; f++) {
        Point2D p0 = projected[faces[f][0]];
        Point2D p1 = projected[faces[f][1]];
        Point2D p2 = projected[faces[f][2]];
        Point2D p3 = projected[faces[f][3]];

        int area = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
        if (area > 0) {
            int intensity = area * 200 / 15000 + 55;
            uint32_t color = apply_light(face_colors[f], intensity);
            draw_triangle(p0.x, p0.y, p0.depth, p1.x, p1.y, p1.depth, p2.x, p2.y, p2.depth, color, z_buffer, screen_w, screen_h);
            draw_triangle(p0.x, p0.y, p0.depth, p2.x, p2.y, p2.depth, p3.x, p3.y, p3.depth, color, z_buffer, screen_w, screen_h);
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

    if (!g_DoubleBufferEnabled) {
        printf("Error: Double buffering not available.\n");
        getch();
        return;
    }
    
    int screen_w = g_vbe_screen->width;
    int screen_h = g_vbe_screen->height;

    // Scene and Camera state
    int angleX = 0, angleY = 0, angleZ = 0;
    int camX = 0, camY = 0, camZ = -300;

    // Dynamic Buffer: Allocate memory for projected points
    Point2D* projected = (Point2D*)malloc(8 * sizeof(Point2D));
    if (!projected) {
        printf("Error: Failed to allocate memory for projection.\n");
        graphics_set_double_buffering(false);
        getch();
        return;
    }

    // Allocate Z-Buffer (Depth Buffer)
    uint32_t* z_buffer = (uint32_t*)malloc(screen_w * screen_h * sizeof(uint32_t));
    if (!z_buffer) {
        printf("Error: Failed to allocate Z-buffer.\n");
        free(projected);
        graphics_set_double_buffering(false);
        getch();
        return;
    }

    // Mask IRQ 1 (Keyboard) to prevent the OS ISR from stealing the keypress
    i686_outb(0x21, i686_inb(0x21) | 0x02);

    // FPS counter variables
    int frame_count = 0;
    uint32_t start_ticks = g_ticks;
    int fps = 0;
    char fps_str[16];

    // Loop until key press
    while (1) { // Main loop
        
        // 1. Clear Buffer (Black)
        graphics_clear_buffer(0x00000000);
        memset(z_buffer, 0xFF, screen_w * screen_h * sizeof(uint32_t)); // Clear Z-buffer to max depth

        // Render main cube (stationary, rotating)
        render_cube(angleX, angleY, angleZ, 0, 0, 0, CUBE_SIZE,
                    camX, camY, camZ, projected, z_buffer, screen_w, screen_h);

        // Calculate orbit for second cube
        int orbit_radius = 150;
        int orbit_angle = (angleY * 2) % 64;
        int orbit_x = (sin_table[orbit_angle] * orbit_radius) / 128;
        int orbit_z = (cos_table[orbit_angle] * orbit_radius) / 128;

        // Render orbiting cube (smaller, rotating faster, orbiting)
        render_cube((angleX * 2) % 64, (angleY * 3) % 64, (angleZ * 4) % 64,
                    orbit_x, 0, orbit_z, CUBE_SIZE / 2,
                    camX, camY, camZ, projected, z_buffer, screen_w, screen_h);

        // Calculate orbit for a third cube around the X-axis
        int orbit_radius_2 = 120;
        int orbit_angle_2 = (angleZ * 3) % 64; // Use a different angle for different speed
        int orbit_y_2 = (sin_table[orbit_angle_2] * orbit_radius_2) / 128;
        int orbit_z_2 = (cos_table[orbit_angle_2] * orbit_radius_2) / 128;

        // Render third cube (even smaller, orbiting on Y-Z plane)
        render_cube((angleX * 4) % 64, (angleY * 2) % 64, (angleZ * 3) % 64,
                    0, orbit_y_2, orbit_z_2, CUBE_SIZE / 3,
                    camX, camY, camZ, projected, z_buffer, screen_w, screen_h);

        // --- FPS Calculation & Display ---
        frame_count++;
        uint32_t current_ticks = g_ticks;
        // Assuming 100Hz timer, 100 ticks = 1 second
        if (current_ticks - start_ticks >= 100) {
            fps = frame_count;
            frame_count = 0;
            start_ticks = current_ticks;
        }
        strcpy(fps_str, "FPS: ");
        itoa(fps, fps_str + 5);
        draw_string(10, 10, fps_str, 0x00FFFFFF); // White text

        draw_string(10, screen_h - 20, "Move: WASD, Space, L-Ctrl | Exit: ESC", 0x00CCCCCC);

        // Wait for Vertical Sync to prevent tearing ("scanning" effect)
        wait_for_vsync();

        // Swap Buffer to Screen
        graphics_swap_buffer();

        // Update angles
        angleX = (angleX + 1) % 64;
        angleY = (angleY + 2) % 64;
        angleZ = (angleZ + 1) % 64;

        // Handle keyboard input for camera movement
        if (i686_inb(0x64) & 0x01) {
            uint8_t scancode = i686_inb(0x60);
            if ((scancode & 0x80) == 0) { // Key Press (Make code)
                int move_speed = 10;
                switch (scancode) {
                    case 0x01: // ESC
                        goto end_loop;
                    case 0x11: // W (forward)
                        camZ += move_speed;
                        break;
                    case 0x1F: // S (backward)
                        camZ -= move_speed;
                        break;
                    case 0x1E: // A (left)
                        camX -= move_speed;
                        break;
                    case 0x20: // D (right)
                        camX += move_speed;
                        break;
                    case 0x39: // Space (up)
                        camY += move_speed;
                        break;
                    case 0x1D: // Left Ctrl (down)
                        camY -= move_speed;
                        break;
                }
            }
        }
    }
end_loop:;
    // Restore IRQ 1 (Unmask Keyboard)
    i686_outb(0x21, i686_inb(0x21) & ~0x02);

    // Free dynamic memory
    free(projected);
    free(z_buffer);

    // Cleanup: Disable double buffering (returns to direct drawing)
    graphics_set_double_buffering(false);
    clrscr(); // Clear screen back to text mode look
}