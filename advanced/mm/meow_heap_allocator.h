/* advanced/mm/meow_heap_allocator.h - Cat-themed Heap Allocator Interface
 *
 * MeowKernel Memory Management - Secure heap allocator with comprehensive
 * error handling and memory safety features.
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_HEAP_ALLOCATOR_H
#define MEOW_HEAP_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>
#include "../hal/meow_hal_interface.h"

/* ============================================================================
 * CAT HEAP CONSTANTS AND CONFIGURATION
 * ============================================================================ */

/* Memory layout constants - eliminated magic numbers */
#define MEOW_HEAP_SIZE_MB           1
#define MEOW_HEAP_SIZE_BYTES        (MEOW_HEAP_SIZE_MB * 1024 * 1024)
#define MEOW_HEAP_START             0x200000  /* 2MB mark - cozy cat territory */
#define MEOW_HEAP_END               (MEOW_HEAP_START + MEOW_HEAP_SIZE_BYTES)

/* Heap configuration constants */
#define MEOW_HEAP_MIN_BLOCK_SIZE    16        /* Minimum allocation size */
#define MEOW_HEAP_ALIGNMENT         4         /* Memory alignment */
#define MEOW_HEAP_MAX_ALLOC_SIZE    (MEOW_HEAP_SIZE_BYTES / 2)  /* Max single allocation */
#define MEOW_HEAP_GUARD_PATTERN     0xDEADBEEF  /* Guard pattern for corruption detection */

/* Heap block flags */
#define MEOW_HEAP_BLOCK_FREE        0x00
#define MEOW_HEAP_BLOCK_OCCUPIED    0x01
#define MEOW_HEAP_BLOCK_GUARD       0x02
#define MEOW_HEAP_BLOCK_CORRUPTED   0xFF

/* ============================================================================
 * CAT HEAP DATA STRUCTURES
 * ============================================================================ */

/**
 * cat_memory_block - Memory block structure (each cat gets their own bed metadata)
 * 
 * This structure represents a memory block in the heap. It contains metadata
 * about the block including size, occupancy status, and pointer to the next block.
 */
typedef struct cat_memory_block {
    uint32_t size;                      /* Size of usable memory in this block */
    uint8_t occupied;                   /* 0 = free, 1 = occupied */
    uint8_t flags;                      /* Block flags for debugging and validation */
    uint16_t magic;                     /* Magic number for corruption detection */
    struct cat_memory_block* next_bed;  /* Pointer to next memory block */
    uint32_t guard_front;               /* Front guard pattern */
    /* User data follows here */
    /* uint32_t guard_back follows user data */
} cat_memory_block_t;

/**
 * cat_heap_stats - Heap statistics structure
 * 
 * Contains comprehensive statistics about the heap state for debugging
 * and monitoring purposes.
 */
typedef struct cat_heap_stats {
    uint32_t total_size;        /* Total heap size */
    uint32_t used_size;         /* Currently used memory */
    uint32_t free_size;         /* Currently free memory */
    uint32_t block_count;       /* Total number of blocks */
    uint32_t free_blocks;       /* Number of free blocks */
    uint32_t occupied_blocks;   /* Number of occupied blocks */
    uint32_t allocations;       /* Total allocation count */
    uint32_t deallocations;     /* Total deallocation count */
    uint32_t failures;          /* Allocation failure count */
    uint32_t corruptions;       /* Detected corruption count */
    uint32_t largest_free;      /* Largest free block size */
    uint32_t smallest_free;     /* Smallest free block size */
    double fragmentation;       /* Heap fragmentation percentage */
} cat_heap_stats_t;

/* ============================================================================
 * CAT HEAP CORE FUNCTIONS
 * ============================================================================ */

/**
 * meow_heap_init - Initialize the cat heap allocator
 * 
 * Initializes the heap allocator with proper memory setup, guard patterns,
 * and initial statistics. Must be called before any allocation operations.
 * 
 * @return MEOW_SUCCESS on success, error code on failure
 */
meow_error_t meow_heap_init(void);

/**
 * meow_heap_shutdown - Shutdown the heap allocator
 * 
 * Cleans up the heap allocator and releases all resources. After calling
 * this function, no further heap operations should be performed.
 * 
 * @return MEOW_SUCCESS on success, error code on failure
 */
meow_error_t meow_heap_shutdown(void);

/**
 * meow_heap_alloc - Allocate memory from the cat heap
 * @size: Number of bytes to allocate
 * 
 * Allocates a memory block of the specified size from the cat heap.
 * The returned memory is aligned and includes guard patterns for
 * corruption detection.
 * 
 * @return Pointer to allocated memory, or NULL on failure
 */
void* meow_heap_alloc(size_t size);

/**
 * meow_heap_free - Free memory back to the cat heap
 * @ptr: Pointer to memory to free
 * 
 * Frees a previously allocated memory block back to the heap.
 * Includes comprehensive validation and corruption detection.
 * 
 * @return MEOW_SUCCESS on success, error code on failure
 */
meow_error_t meow_heap_free(void* ptr);

/**
 * meow_heap_realloc - Reallocate memory with new size
 * @ptr: Pointer to existing memory (or NULL)
 * @new_size: New size for the memory block
 * 
 * Reallocates memory to a new size. If ptr is NULL, behaves like
 * meow_heap_alloc. If new_size is 0, behaves like meow_heap_free.
 * 
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* meow_heap_realloc(void* ptr, size_t new_size);

/**
 * meow_heap_calloc - Allocate and zero-initialize memory
 * @count: Number of elements
 * @size: Size of each element
 * 
 * Allocates memory for an array of count elements of size bytes each
 * and initializes all bytes to zero. Includes overflow protection.
 * 
 * @return Pointer to allocated and zeroed memory, or NULL on failure
 */
void* meow_heap_calloc(size_t count, size_t size);

/* ============================================================================
 * CAT HEAP STATISTICS AND MONITORING
 * ============================================================================ */

/**
 * meow_heap_get_stats - Get comprehensive heap statistics
 * @stats: Pointer to structure to fill with statistics
 * 
 * Retrieves detailed statistics about the current heap state including
 * memory usage, fragmentation, and allocation history.
 * 
 * @return MEOW_SUCCESS on success, error code on failure
 */
meow_error_t meow_heap_get_stats(cat_heap_stats_t* stats);

/**
 * meow_heap_print_stats - Print heap statistics to debug output
 * 
 * Prints a formatted summary of heap statistics including memory usage,
 * fragmentation, and block information.
 */
void meow_heap_print_stats(void);

/**
 * meow_heap_get_block_size - Get the size of an allocated block
 * @ptr: Pointer to allocated memory
 * 
 * Returns the actual allocated size of the memory block pointed to by ptr.
 * Includes validation to ensure the pointer is valid.
 * 
 * @return Block size in bytes, or 0 on error
 */
size_t meow_heap_get_block_size(const void* ptr);

/**
 * meow_heap_get_utilization - Get current heap utilization percentage
 * 
 * Calculates and returns the current heap utilization as a percentage
 * of total heap space.
 * 
 * @return Utilization percentage (0-100), or -1 on error
 */
int meow_heap_get_utilization(void);

/* ============================================================================
 * CAT HEAP VALIDATION AND DEBUGGING
 * ============================================================================ */

/**
 * meow_heap_validate - Validate entire heap integrity
 * 
 * Performs comprehensive validation of the entire heap structure,
 * checking for corruption, invalid pointers, and consistency errors.
 * 
 * @return MEOW_SUCCESS if heap is valid, error code if corruption detected
 */
meow_error_t meow_heap_validate(void);

/**
 * meow_heap_validate_pointer - Validate a specific pointer
 * @ptr: Pointer to validate
 * 
 * Validates that a pointer points to a valid allocated block in the heap
 * and checks for corruption of the block's metadata.
 * 
 * @return MEOW_SUCCESS if pointer is valid, error code otherwise
 */
meow_error_t meow_heap_validate_pointer(const void* ptr);

/**
 * meow_heap_dump_blocks - Dump all heap blocks for debugging
 * 
 * Prints detailed information about all blocks in the heap including
 * their sizes, status, addresses, and guard patterns.
 */
void meow_heap_dump_blocks(void);

/**
 * meow_heap_check_corruption - Check for heap corruption
 * 
 * Scans the heap for signs of corruption including invalid magic numbers,
 * corrupted guard patterns, and inconsistent block sizes.
 * 
 * @return Number of corrupted blocks found
 */
uint32_t meow_heap_check_corruption(void);

/* ============================================================================
 * CAT HEAP MAINTENANCE FUNCTIONS
 * ============================================================================ */

/**
 * meow_heap_defragment - Defragment the heap by merging free blocks
 * 
 * Merges adjacent free blocks to reduce fragmentation and improve
 * allocation efficiency. Updates heap statistics after defragmentation.
 * 
 * @return Number of blocks merged, or negative error code on failure
 */
int meow_heap_defragment(void);

/**
 * meow_heap_compact - Compact the heap by moving allocated blocks
 * 
 * Advanced heap compaction that moves allocated blocks to reduce
 * fragmentation. This operation is expensive and may not be suitable
 * for real-time systems.
 * 
 * @return MEOW_SUCCESS on success, error code on failure
 */
meow_error_t meow_heap_compact(void);

/**
 * meow_heap_get_fragmentation - Calculate heap fragmentation percentage
 * 
 * Calculates the heap fragmentation as a percentage based on the
 * ratio of free block count to total free space.
 * 
 * @return Fragmentation percentage (0.0 = no fragmentation, 100.0 = highly fragmented)
 */
double meow_heap_get_fragmentation(void);

/* ============================================================================
 * LEGACY COMPATIBILITY FUNCTIONS
 * ============================================================================ */

/* These functions are provided for backward compatibility with existing code */

/**
 * setup_cat_heap - Legacy heap initialization function
 * @deprecated Use meow_heap_init() instead
 */
__attribute__((deprecated("Use meow_heap_init() instead")))
static inline void setup_cat_heap(void) {
    meow_heap_init();
}

/**
 * meow_alloc - Legacy allocation function
 * @size: Size to allocate
 * @deprecated Use meow_heap_alloc() instead
 */
__attribute__((deprecated("Use meow_heap_alloc() instead")))
static inline void* meow_alloc(size_t size) {
    return meow_heap_alloc(size);
}

/**
 * meow_free - Legacy free function
 * @ptr: Pointer to free
 * @deprecated Use meow_heap_free() instead
 */
__attribute__((deprecated("Use meow_heap_free() instead")))
static inline void meow_free(void* ptr) {
    meow_heap_free(ptr);
}

/**
 * meow_realloc - Legacy realloc function
 * @ptr: Pointer to reallocate
 * @size: New size
 * @deprecated Use meow_heap_realloc() instead
 */
__attribute__((deprecated("Use meow_heap_realloc() instead")))
static inline void* meow_realloc(void* ptr, size_t size) {
    return meow_heap_realloc(ptr, size);
}

/**
 * meow_calloc - Legacy calloc function
 * @count: Number of elements
 * @size: Size of each element
 * @deprecated Use meow_heap_calloc() instead
 */
__attribute__((deprecated("Use meow_heap_calloc() instead")))
static inline void* meow_calloc(size_t count, size_t size) {
    return meow_heap_calloc(count, size);
}

/* ============================================================================
 * CAT HEAP CONFIGURATION AND TUNING
 * ============================================================================ */

/**
 * meow_heap_set_debug_level - Set heap debugging level
 * @level: Debug level (0 = none, 1 = basic, 2 = verbose, 3 = paranoid)
 * 
 * Controls the amount of debugging information and validation performed
 * by the heap allocator. Higher levels provide more safety but reduce performance.
 * 
 * @return Previous debug level
 */
uint8_t meow_heap_set_debug_level(uint8_t level);

/**
 * meow_heap_enable_guards - Enable/disable guard pattern checking
 * @enable: 1 to enable, 0 to disable
 * 
 * Controls whether guard patterns are checked for corruption detection.
 * Disabling can improve performance but reduces safety.
 * 
 * @return Previous setting
 */
uint8_t meow_heap_enable_guards(uint8_t enable);

/* ============================================================================
 * CAT HEAP UTILITY MACROS
 * ============================================================================ */

/* Memory alignment macros */
#define MEOW_HEAP_ALIGN(size) \
    (((size) + (MEOW_HEAP_ALIGNMENT - 1)) & ~(MEOW_HEAP_ALIGNMENT - 1))

#define MEOW_HEAP_IS_ALIGNED(ptr) \
    (((uintptr_t)(ptr) & (MEOW_HEAP_ALIGNMENT - 1)) == 0)

/* Block validation macros */
#define MEOW_HEAP_MAGIC_VALUE       0xCAFE
#define MEOW_HEAP_IS_VALID_BLOCK(block) \
    ((block) && (block)->magic == MEOW_HEAP_MAGIC_VALUE && \
     (block)->size > 0 && (block)->size <= MEOW_HEAP_MAX_ALLOC_SIZE)

/* Size validation macros */
#define MEOW_HEAP_IS_VALID_SIZE(size) \
    ((size) > 0 && (size) <= MEOW_HEAP_MAX_ALLOC_SIZE)

/* Overflow detection macros */
#define MEOW_HEAP_WILL_OVERFLOW_ADD(a, b) \
    ((a) > SIZE_MAX - (b))

#define MEOW_HEAP_WILL_OVERFLOW_MUL(a, b) \
    ((a) != 0 && (b) > SIZE_MAX / (a))

#endif /* MEOW_HEAP_ALLOCATOR_H */