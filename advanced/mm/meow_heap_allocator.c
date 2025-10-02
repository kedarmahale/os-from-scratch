/* advanced/mm/meow_heap_allocator.c - Cat-themed Heap Allocator Implementation (Fixed)
 *
 * Clean heap implementation with NO function redefinitions and correct constants
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_heap_allocator.h"
#include "meow_memory_manager.h"
#include "../../kernel/meow_util.h"

/* ============================================================================
 * CAT HEAP GLOBAL STATE
 * ============================================================================ */

static cat_memory_block_t* first_cat_bed = NULL;
static uint32_t heap_total_size = 0;
static uint32_t heap_used_size = 0;
static uint32_t heap_free_size = 0;
static uint32_t heap_block_count = 0;
static uint8_t heap_initialized = 0;

/* Cat-themed statistics */
static cat_heap_stats_t heap_stats = {0};

/* ============================================================================
 * FORWARD DECLARATIONS (Functions used before defined)
 * ============================================================================ */

static void merge_free_blocks_internal(void);
static cat_memory_block_t* find_free_block_internal(size_t size);
static meow_error_t validate_pointer_internal(const void* ptr);

/* ============================================================================
 * MAIN HEAP INTERFACE FUNCTIONS (ONLY THESE - NO REDEFINITIONS)
 * ============================================================================ */

/**
 * meow_heap_init - Initialize the cat heap (THE ONLY initialization function)
 */
meow_error_t meow_heap_init(void) {
    if (heap_initialized) {
        meow_log(MEOW_LOG_HISS, "Cat heap already initialized!");
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }

    meow_log(MEOW_LOG_CHIRP, "Initializing cat heap allocator...");

    /* Initialize the first cat bed (memory block) */
    first_cat_bed = (cat_memory_block_t*)MEOW_HEAP_START;
    first_cat_bed->size = MEOW_HEAP_SIZE_BYTES - sizeof(cat_memory_block_t);
    first_cat_bed->occupied = 0; /* No cat sleeping yet */
    first_cat_bed->flags = MEOW_HEAP_BLOCK_FREE;
    first_cat_bed->magic = MEOW_HEAP_MAGIC_VALUE;
    first_cat_bed->next_bed = NULL;
    first_cat_bed->guard_front = MEOW_HEAP_GUARD_PATTERN;

    /* Initialize heap statistics */
    heap_total_size = MEOW_HEAP_SIZE_BYTES;
    heap_used_size = sizeof(cat_memory_block_t); /* Header block */
    heap_free_size = first_cat_bed->size;
    heap_block_count = 1;
    heap_initialized = 1;

    /* Initialize comprehensive statistics */
    heap_stats.total_size = heap_total_size;
    heap_stats.used_size = heap_used_size;
    heap_stats.free_size = heap_free_size;
    heap_stats.block_count = heap_block_count;
    heap_stats.free_blocks = 1;
    heap_stats.occupied_blocks = 0;
    heap_stats.allocations = 0;
    heap_stats.deallocations = 0;
    heap_stats.failures = 0;
    heap_stats.corruptions = 0;

    meow_log(MEOW_LOG_CHIRP, "Cat heap initialized: %d KB at 0x%08x",
             heap_total_size / 1024, MEOW_HEAP_START);
    
    return MEOW_SUCCESS;
}

/**
 * meow_heap_shutdown - Shutdown the heap allocator
 */
meow_error_t meow_heap_shutdown(void) {
    if (!heap_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    meow_log(MEOW_LOG_CHIRP, "Shutting down cat heap allocator");

    /* Validate heap before shutdown */
    meow_error_t validation_result = meow_heap_validate();
    if (validation_result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_HISS, "Heap corruption detected during shutdown!");
    }

    /* Reset all state */
    first_cat_bed = NULL;
    heap_total_size = 0;
    heap_used_size = 0;
    heap_free_size = 0;
    heap_block_count = 0;
    heap_initialized = 0;
    
    /* Clear statistics */
    meow_memset(&heap_stats, 0, sizeof(heap_stats));

    meow_log(MEOW_LOG_CHIRP, "Cat heap shutdown complete");
    return MEOW_SUCCESS;
}

/**
 * meow_heap_alloc - Allocate memory from the cat heap (THE ONLY alloc function)
 */
void* meow_heap_alloc(size_t size) {
    /* Input validation */
    if (size == 0 || size > MEOW_HEAP_MAX_ALLOC_SIZE) {
        meow_log(MEOW_LOG_YOWL, "Invalid allocation size: %zu", size);
        heap_stats.failures++;
        return NULL;
    }

    if (!heap_initialized) {
        meow_error_t init_result = meow_heap_init();
        if (init_result != MEOW_SUCCESS) {
            meow_log(MEOW_LOG_YOWL, "Failed to initialize heap for allocation");
            heap_stats.failures++;
            return NULL;
        }
    }

    /* Align size to boundary (cats like neat arrangements) */
    size = MEOW_HEAP_ALIGN(size);
    if (size < MEOW_HEAP_MIN_BLOCK_SIZE) {
        size = MEOW_HEAP_MIN_BLOCK_SIZE;
    }

    /* Find a suitable free block */
    cat_memory_block_t* block = find_free_block_internal(size);
    if (!block) {
        meow_log(MEOW_LOG_YOWL, "No suitable free block found for size %zu", size);
        heap_stats.failures++;
        return NULL;
    }

    /* Split block if it's too big */
    if (block->size > size + sizeof(cat_memory_block_t) + MEOW_HEAP_MIN_BLOCK_SIZE) {
        uintptr_t new_block_addr = (uintptr_t)block + sizeof(cat_memory_block_t) + size;
        
        /* Validate the new block address */
        if (new_block_addr >= MEOW_HEAP_END || new_block_addr < (uintptr_t)block) {
            meow_log(MEOW_LOG_YOWL, "Block split would cause overflow!");
            heap_stats.failures++;
            return NULL;
        }

        /* Create new free block */
        cat_memory_block_t* new_block = (cat_memory_block_t*)new_block_addr;
        new_block->size = block->size - size - sizeof(cat_memory_block_t);
        new_block->occupied = 0;
        new_block->flags = MEOW_HEAP_BLOCK_FREE;
        new_block->magic = MEOW_HEAP_MAGIC_VALUE;
        new_block->guard_front = MEOW_HEAP_GUARD_PATTERN;
        new_block->next_bed = block->next_bed;

        /* Update original block */
        block->next_bed = new_block;
        block->size = size;
        heap_block_count++;
        heap_stats.free_blocks++;
    }

    /* Mark block as occupied */
    block->occupied = 1;
    block->flags = MEOW_HEAP_BLOCK_OCCUPIED;

    /* Update statistics */
    heap_used_size += size + sizeof(cat_memory_block_t);
    heap_free_size -= size + sizeof(cat_memory_block_t);
    heap_stats.allocations++;
    heap_stats.used_size = heap_used_size;
    heap_stats.free_size = heap_free_size;
    heap_stats.occupied_blocks++;
    if (heap_stats.free_blocks > 0) {
        heap_stats.free_blocks--;
    }

    /* Return pointer to user data area */
    void* user_ptr = (void*)((uint8_t*)block + sizeof(cat_memory_block_t));
    
    meow_log(MEOW_LOG_PURR, "Cat found cozy space at 0x%08x (%zu bytes)",
             (uint32_t)user_ptr, size);
    
    return user_ptr;
}

/**
 * meow_heap_free - Free memory back to the cat heap (THE ONLY free function)
 */
meow_error_t meow_heap_free(void* ptr) {
    if (!ptr) {
        return MEOW_SUCCESS; /* Free NULL is always safe */
    }

    if (!heap_initialized) {
        meow_log(MEOW_LOG_YOWL, "Attempting to free from uninitialized heap");
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    /* Validate the pointer */
    meow_error_t validation = validate_pointer_internal(ptr);
    if (validation != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "Invalid pointer passed to meow_heap_free: 0x%08x", (uint32_t)ptr);
        heap_stats.corruptions++;
        return validation;
    }

    /* Get the block header */
    cat_memory_block_t* block = (cat_memory_block_t*)((uint8_t*)ptr - sizeof(cat_memory_block_t));
    
    /* Validate block */
    if (!block->occupied || block->magic != MEOW_HEAP_MAGIC_VALUE) {
        meow_log(MEOW_LOG_YOWL, "Attempting to free already free or corrupted block!");
        heap_stats.corruptions++;
        return MM_ERROR_HEAP_CORRUPTION;
    }

    /* Mark block as free */
    block->occupied = 0;
    block->flags = MEOW_HEAP_BLOCK_FREE;

    /* Update statistics */
    heap_used_size -= block->size + sizeof(cat_memory_block_t);
    heap_free_size += block->size + sizeof(cat_memory_block_t);
    heap_stats.deallocations++;
    heap_stats.used_size = heap_used_size;
    heap_stats.free_size = heap_free_size;
    heap_stats.occupied_blocks--;
    heap_stats.free_blocks++;

    meow_log(MEOW_LOG_PURR, "Cat left their space at 0x%08x", (uint32_t)ptr);

    /* Merge adjacent free blocks */
    merge_free_blocks_internal();

    return MEOW_SUCCESS;
}

/**
 * meow_heap_realloc - Reallocate memory (THE ONLY realloc function)
 */
void* meow_heap_realloc(void* ptr, size_t new_size) {
    /* Handle special cases */
    if (!ptr) {
        return meow_heap_alloc(new_size);
    }
    
    if (new_size == 0) {
        meow_heap_free(ptr);
        return NULL;
    }

    if (new_size > MEOW_HEAP_MAX_ALLOC_SIZE) {
        meow_log(MEOW_LOG_YOWL, "Realloc size too large: %zu", new_size);
        return NULL;
    }

    /* Validate the existing pointer */
    if (validate_pointer_internal(ptr) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "Invalid pointer in realloc: 0x%08x", (uint32_t)ptr);
        return NULL;
    }

    /* Get current block */
    cat_memory_block_t* block = (cat_memory_block_t*)((uint8_t*)ptr - sizeof(cat_memory_block_t));
    size_t old_size = block->size;

    /* Align new size */
    new_size = MEOW_HEAP_ALIGN(new_size);

    /* If current block is large enough, just return it */
    if (old_size >= new_size) {
        return ptr;
    }

    /* Need to allocate new block */
    void* new_ptr = meow_heap_alloc(new_size);
    if (new_ptr) {
        /* Copy old data */
        size_t copy_size = (old_size < new_size) ? old_size : new_size;
        meow_memcpy(new_ptr, ptr, copy_size);
        meow_heap_free(ptr);
    }

    return new_ptr;
}

/**
 * meow_heap_calloc - Allocate and zero-initialize memory (THE ONLY calloc function)
 */
void* meow_heap_calloc(size_t count, size_t size) {
    /* Check for overflow */
    if (count != 0 && size > SIZE_MAX / count) {
        meow_log(MEOW_LOG_YOWL, "Calloc size overflow: count %zu, size %zu", count, size);
        heap_stats.failures++;
        return NULL;
    }

    size_t total_size = count * size;
    void* ptr = meow_heap_alloc(total_size);
    
    if (ptr) {
        /* Zero the memory */
        meow_memset(ptr, 0, total_size);
    }

    return ptr;
}

/* ============================================================================
 * HEAP STATISTICS AND MONITORING
 * ============================================================================ */

/**
 * meow_heap_get_stats - Get comprehensive heap statistics
 */
meow_error_t meow_heap_get_stats(cat_heap_stats_t* stats) {
    if (!stats) {
        return MEOW_ERROR_NULL_POINTER;
    }

    if (!heap_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    /* Update dynamic statistics */
    heap_stats.total_size = heap_total_size;
    heap_stats.used_size = heap_used_size;
    heap_stats.free_size = heap_free_size;
    heap_stats.block_count = heap_block_count;

    /* Calculate fragmentation */
    if (heap_stats.free_blocks > 0 && heap_stats.free_size > 0) {
        heap_stats.fragmentation = ((double)heap_stats.free_blocks / (double)(heap_stats.free_size / MEOW_HEAP_MIN_BLOCK_SIZE)) * 100.0;
    } else {
        heap_stats.fragmentation = 0.0;
    }

    /* Copy statistics */
    *stats = heap_stats;
    return MEOW_SUCCESS;
}

/**
 * meow_heap_print_stats - Print heap statistics
 */
void meow_heap_print_stats(void) {
    if (!heap_initialized) {
        meow_printf("Cat heap not initialized yet\n");
        return;
    }

    meow_printf("\n==== CAT HEAP STATISTICS ====\n");
    meow_printf("===============================\n");
    meow_printf("Total Size:      %u bytes (%u KB)\n", heap_total_size, heap_total_size / 1024);
    meow_printf("Used Size:       %u bytes (%u KB)\n", heap_used_size, heap_used_size / 1024);
    meow_printf("Free Size:       %u bytes (%u KB)\n", heap_free_size, heap_free_size / 1024);
    meow_printf("Block Count:     %u blocks\n", heap_block_count);
    meow_printf("Free Blocks:     %u blocks\n", heap_stats.free_blocks);
    meow_printf("Occupied Blocks: %u blocks\n", heap_stats.occupied_blocks);
    meow_printf("Allocations:     %u total\n", heap_stats.allocations);
    meow_printf("Deallocations:   %u total\n", heap_stats.deallocations);
    meow_printf("Failures:        %u total\n", heap_stats.failures);
    meow_printf("Corruptions:     %u detected\n", heap_stats.corruptions);
    meow_printf("Utilization:     %u%%\n", heap_total_size > 0 ? (heap_used_size * 100) / heap_total_size : 0);
    meow_printf("Fragmentation:   %.2f%%\n", heap_stats.fragmentation);
    meow_printf("================================\n\n");
}

/* ============================================================================
 * HEAP VALIDATION AND DEBUGGING
 * ============================================================================ */

/**
 * meow_heap_validate - Validate entire heap integrity
 */
meow_error_t meow_heap_validate(void) {
    if (!heap_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    meow_log(MEOW_LOG_PURR, "Validating heap integrity...");

    cat_memory_block_t* current = first_cat_bed;
    uint32_t blocks_found = 0;
    uint32_t corruptions = 0;

    while (current) {
        blocks_found++;

        /* Validate block magic */
        if (current->magic != MEOW_HEAP_MAGIC_VALUE) {
            meow_log(MEOW_LOG_YOWL, "Corrupted magic number at block 0x%08x", (uint32_t)current);
            corruptions++;
        }

        /* Validate block size */
        if (current->size == 0 || current->size > MEOW_HEAP_SIZE_BYTES) {
            meow_log(MEOW_LOG_YOWL, "Invalid block size %u at 0x%08x", current->size, (uint32_t)current);
            corruptions++;
        }

        /* Validate guard pattern */
        if (current->guard_front != MEOW_HEAP_GUARD_PATTERN) {
            meow_log(MEOW_LOG_YOWL, "Corrupted guard pattern at 0x%08x", (uint32_t)current);
            corruptions++;
        }

        current = current->next_bed;

        /* Prevent infinite loops */
        if (blocks_found > heap_block_count * 2) {
            meow_log(MEOW_LOG_YOWL, "Potential infinite loop in heap chain!");
            corruptions++;
            break;
        }
    }

    if (blocks_found != heap_block_count) {
        meow_log(MEOW_LOG_YOWL, "Block count mismatch: found %u, expected %u", blocks_found, heap_block_count);
        corruptions++;
    }

    heap_stats.corruptions += corruptions;

    if (corruptions > 0) {
        meow_log(MEOW_LOG_YOWL, "Heap validation failed: %u corruptions detected", corruptions);
        return MM_ERROR_HEAP_CORRUPTION;
    }

    meow_log(MEOW_LOG_PURR, "Heap integrity validation passed: %u blocks verified", blocks_found);
    return MEOW_SUCCESS;
}

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================ */

/**
 * find_free_block_internal - Find a free block of suitable size
 */
static cat_memory_block_t* find_free_block_internal(size_t size) {
    cat_memory_block_t* current = first_cat_bed;
    
    while (current) {
        if (!current->occupied && current->size >= size) {
            return current;
        }
        current = current->next_bed;
    }
    
    return NULL;
}

/**
 * validate_pointer_internal - Validate a user pointer
 */
static meow_error_t validate_pointer_internal(const void* ptr) {
    if (!ptr) {
        return MEOW_ERROR_NULL_POINTER;
    }

    uintptr_t addr = (uintptr_t)ptr;
    
    /* Check if pointer is within heap bounds */
    if (addr < MEOW_HEAP_START + sizeof(cat_memory_block_t) || addr >= MEOW_HEAP_END) {
        return MM_ERROR_INVALID_ADDRESS;
    }

    return MEOW_SUCCESS;
}

/**
 * merge_free_blocks_internal - Merge adjacent free blocks
 */
static void merge_free_blocks_internal(void) {
    if (!heap_initialized) {
        return;
    }

    cat_memory_block_t* current = first_cat_bed;
    uint32_t merges = 0;

    while (current && current->next_bed) {
        if (!current->occupied && !current->next_bed->occupied) {
            /* Check if blocks are adjacent */
            uint8_t* current_end = (uint8_t*)current + sizeof(cat_memory_block_t) + current->size;
            uint8_t* next_start = (uint8_t*)current->next_bed;

            if (current_end == next_start) {
                /* Blocks are adjacent, merge them */
                current->size += current->next_bed->size + sizeof(cat_memory_block_t);
                current->next_bed = current->next_bed->next_bed;
                heap_block_count--;
                heap_stats.free_blocks--;
                merges++;
                continue; /* Check again with same current block */
            }
        }
        current = current->next_bed;
    }

    if (merges > 0) {
        meow_log(MEOW_LOG_PURR, "Merged %u adjacent free blocks", merges);
    }
}