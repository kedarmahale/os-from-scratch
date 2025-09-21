#ifndef CAT_HEAP_H
#define CAT_HEAP_H

#include <stdint.h>
#include <stddef.h>

// =============================================================================
// CAT HEAP ALLOCATOR
// =============================================================================

// Constants for cat heap management
#define CAT_HEAP_START 0x200000     // 2MB mark - cozy cat heap location
#define CAT_HEAP_SIZE 0x100000      // 1MB heap (plenty of room for cats)

// =============================================================================
// CAT MEMORY BLOCK STRUCTURE
// =============================================================================

// Cat memory block structure (like a cat bed)
typedef struct cat_memory_block {
    uint32_t size;
    uint32_t occupied;              // Is a cat sleeping here?
    struct cat_memory_block* next_bed;
} cat_memory_block_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

// Set up the cat heap (prepare cozy sleeping areas)
void setup_cat_heap(void);

// Allocate memory for cats (give them a cozy space)
void* meow_alloc(size_t size);

// Free memory (cat leaves their cozy space)
void meow_free(void* ptr);

// Heap information functions
uint32_t get_heap_total_size(void);
uint32_t get_heap_used_size(void);
uint32_t get_heap_free_size(void);
uint32_t get_heap_block_count(void);

// Heap debug and status functions
void cat_heap_status(void);
void print_heap_blocks(void);
void validate_heap_integrity(void);

// Heap management functions
void defragment_cat_heap(void);
cat_memory_block_t* find_free_block(size_t size);
void merge_free_blocks(void);

// Memory utilities
void* meow_realloc(void* ptr, size_t new_size);
void* meow_calloc(size_t count, size_t size);

#endif // CAT_HEAP_H

