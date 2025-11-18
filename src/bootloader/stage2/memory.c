#include "memory.h"

void* memcpy(void* dst, const void* src, uint32_t num)
{
    uint8_t* u8_dst = (uint8_t*)dst;
    const uint8_t* u8_src = (const uint8_t*)src;

    // Handle unaligned start
    while ((uintptr_t)u8_dst % 4 != 0 && num > 0) {
        *u8_dst++ = *u8_src++;
        num--;
    }

    // Now, the destination is aligned. Copy 4 bytes at a time.
    uint32_t* u32_dst = (uint32_t*)u8_dst;
    const uint32_t* u32_src = (const uint32_t*)u8_src;
    uint32_t dword_count = num / 4;

    for (uint32_t i = 0; i < dword_count; i++) {
        u32_dst[i] = u32_src[i];
    }

    // Handle any remaining bytes
    u8_dst = (uint8_t*)(u32_dst + dword_count);
    u8_src = (const uint8_t*)(u32_src + dword_count);
    uint32_t remaining_bytes = num % 4;

    for (uint32_t i = 0; i < remaining_bytes; i++) {
        u8_dst[i] = u8_src[i];
    }

    return dst; // Per C standard, return the original destination pointer
}

void * memset(void * ptr, int value, uint32_t num)
{
    uint8_t* u8_ptr = (uint8_t*)ptr;
    uint8_t u8_value = (uint8_t)value;

    // Set bytes until the pointer is 4-byte aligned
    while ((uintptr_t)u8_ptr % 4 != 0 && num > 0) {
        *u8_ptr++ = u8_value;
        num--;
    }

    // Now, the pointer is aligned. Set 4 bytes at a time.
    uint32_t* u32_ptr = (uint32_t*)u8_ptr;
    uint32_t u32_value = (u8_value << 24) | (u8_value << 16) | (u8_value << 8) | u8_value;
    uint32_t dword_count = num / 4;

    for (uint32_t i = 0; i < dword_count; i++) {
        u32_ptr[i] = u32_value;
    }

    // Handle any remaining bytes
    u8_ptr = (uint8_t*)(u32_ptr + dword_count);
    uint32_t remaining_bytes = num % 4;
    for (uint32_t i = 0; i < remaining_bytes; i++) {
        u8_ptr[i] = u8_value;
    }

    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, uint32_t num)
{
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;

    for (uint32_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}