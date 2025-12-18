#include "heap.h"
#include "memory.h"
#include "stdbool.h"

// A simple linked-list based memory allocator

typedef struct block_header {
    size_t size;
    bool is_free;
    struct block_header* next;
} block_header_t;

// The linker provides this symbol, which marks the end of the kernel's code/data.
extern uint8_t __end;

// The start of our heap, which is a linked list of memory blocks
static block_header_t* heap_start = NULL;

// Define a fixed size for the heap (e.g., 4MB)
#define HEAP_SIZE (1024 * 1024 * 4)

void heap_initialize() {
    // The heap starts right after the kernel's end address.
    heap_start = (block_header_t*)&__end;

    // Initially, we have one large free block
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
}

void* malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    block_header_t* current = heap_start;
    while (current) {
        // Find the first block that is free and large enough
        if (current->is_free && current->size >= size) {
            // Is the block large enough to be split?
            if (current->size > size + sizeof(block_header_t)) {
                // Create a new header for the remaining part of the block
                block_header_t* new_block = (block_header_t*)((uint8_t*)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->is_free = true;
                new_block->next = current->next;

                // Update the current block
                current->size = size;
                current->next = new_block;
            }

            current->is_free = false;
            // Return a pointer to the memory region *after* the header
            return (void*)((uint8_t*)current + sizeof(block_header_t));
        }
        current = current->next;
    }

    // No suitable block found
    return NULL;
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }

    // Get the block header from the pointer
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->is_free = true;

    // TODO: Implement coalescing (merging) of adjacent free blocks for better memory usage.
    // For now, we just mark it as free.
}

void* realloc(void* ptr, size_t new_size) {
    if (!ptr) {
        // If ptr is NULL, realloc is equivalent to malloc
        return malloc(new_size);
    }

    if (new_size == 0) {
        // If new_size is 0, realloc is equivalent to free
        free(ptr);
        return NULL;
    }

    // Get the header of the old block
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    
    // If the new size is smaller or equal, we can just return the same pointer for now.
    // A more advanced implementation would shrink the block.
    if (new_size <= header->size) {
        return ptr;
    }

    // Allocate a new, larger block
    void* new_ptr = malloc(new_size);
    memcpy(new_ptr, ptr, header->size); // Copy data from the old block
    free(ptr); // Free the old block
    return new_ptr;
}