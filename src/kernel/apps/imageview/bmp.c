#include "bmp.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "fat.h"
#include "graphics.h"
#include "vbe.h"
#include "globals.h"
#include "heap.h"
#include <arch/i686/keyboard.h>

// works, loads most of file then crashes, likely a memory or FAT issue as the big one (5mb) insta-crashes but smaller ones work a bit.
// Ill look tommorow, review at coding club IT DOESNT CRASH, STILL WEIRD THO

// BMP File Header (14 bytes)
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;      // Magic number "BM"
    uint32_t bfSize;      // File size
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;   // Offset to pixel data
} BMPFileHeader;

// BMP Info Header (40 bytes standard)
typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

// Helper for absolute value
#define ABS(x) ((x) < 0 ? -(x) : (x))

void bmp_view(const char* filename) {
    FAT_File* fd = FAT_Open(&g_Disk, filename, FAT_OPEN_MODE_READ);
    if (!fd) {
        printf("BMP: Could not open file '%s'\n", filename);
        return;
    }

    BMPFileHeader fileHeader;
    if (FAT_Read(&g_Disk, fd, sizeof(BMPFileHeader), &fileHeader) != sizeof(BMPFileHeader)) {
        printf("BMP: Error reading file header.\n");
        FAT_Close(&g_Disk, fd);
        return;
    }

    if (fileHeader.bfType != 0x4D42) { // 'B' 'M' in little endian
        printf("BMP: Not a valid BMP file (Magic: %x)\n", fileHeader.bfType);
        FAT_Close(&g_Disk, fd);
        return;
    }

    BMPInfoHeader infoHeader;
    if (FAT_Read(&g_Disk, fd, sizeof(BMPInfoHeader), &infoHeader) != sizeof(BMPInfoHeader)) {
        printf("BMP: Error reading info header.\n");
        FAT_Close(&g_Disk, fd);
        return;
    }

    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        printf("BMP: Unsupported bit depth %d. Only 24 and 32 bpp supported.\n", infoHeader.biBitCount);
        FAT_Close(&g_Disk, fd);
        return;
    }

    // Seek to the start of pixel data
    if (!FAT_Seek(&g_Disk, fd, fileHeader.bfOffBits)) {
        printf("BMP: Failed to seek to pixel data.\n");
        FAT_Close(&g_Disk, fd);
        return;
    }

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int absHeight = ABS(height);
    int bytesPerPixel = infoHeader.biBitCount / 8;

    // BMP rows are padded to multiples of 4 bytes
    int rowSize = ((width * infoHeader.biBitCount + 31) / 32) * 4;
    
    uint8_t* rowBuffer = (uint8_t*)malloc(rowSize);
    if (!rowBuffer) {
        printf("BMP: Out of memory (row buffer).\n");
        FAT_Close(&g_Disk, fd);
        return;
    }

    // Clear screen to black
    graphics_clear_buffer(0x00000000);

    // Calculate centering offsets
    int startX = (g_vbe_screen->width - width) / 2;
    int startY = (g_vbe_screen->height - absHeight) / 2;

    // Read and draw row by row
    for (int i = 0; i < absHeight; i++) {
        FAT_Read(&g_Disk, fd, rowSize, rowBuffer);

        // BMPs are usually stored bottom-up (positive height)
        // If height is negative, it's top-down.
        int drawY = (height > 0) ? (startY + absHeight - 1 - i) : (startY + i);

        for (int x = 0; x < width; x++) {
            int offset = x * bytesPerPixel;
            uint8_t b = rowBuffer[offset];
            uint8_t g = rowBuffer[offset + 1];
            uint8_t r = rowBuffer[offset + 2];
            
            // Combine to 0x00RRGGBB
            uint32_t color = (r << 16) | (g << 8) | b;
            draw_pixel(startX + x, drawY, color);
        }
    }

    if (g_DoubleBufferEnabled) {
        graphics_swap_buffer();
    }

    free(rowBuffer);
    FAT_Close(&g_Disk, fd);

    // Wait for user input to exit
    getch();
    clrscr(); // Restore console
}