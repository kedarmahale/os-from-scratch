/* advanced/mm/meow_memory_manager.c - MeowKernel Memory Management (Fixed)
 *
 * Clean memory management implementation with correct function names and logging
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_memory_manager.h"
#include "meow_memory_mapper.h"
#include "meow_physical_memory.h" 
#include "meow_heap_allocator.h"
#include "../../kernel/meow_util.h"
#include "../hal/meow_hal_interface.h"

/* ============================================================================
 * GLOBAL MEMORY MANAGEMENT STATE
 * ============================================================================ */

static uint8_t mm_initialized = 0;
static memory_stats_t current_stats = {0};
static mm_error_t last_error = MM_SUCCESS;

/* Forward declarations */
static uint8_t run_memory_validation_tests(void);

/* ============================================================================
 * HAL WRAPPER FUNCTIONS (Missing functions that were causing errors)
 * ============================================================================ */

/**
 * hal_memory_get_available_size - Get available memory size from HAL
 * This wrapper function was missing and causing compilation errors
 */
static inline uint32_t hal_memory_get_available_size(void) {
    const hal_ops_t* ops = hal_get_ops();
    if (ops && ops->memory_ops && ops->memory_ops->get_available_size) {
        return ops->memory_ops->get_available_size();
    }
    /* Fallback: assume 90% of total memory is available */
    return (hal_memory_get_total_size() * 9) / 10;
}

/* ============================================================================
 * HEAP STATISTICS WRAPPER FUNCTIONS (Missing functions)
 * ============================================================================ */

/**
 * get_heap_total_size - Get total heap size (wrapper for new interface)
 */
static inline uint32_t get_heap_total_size(void) {
    cat_heap_stats_t stats = {0};
    if (meow_heap_get_stats(&stats) == MEOW_SUCCESS) {
        return stats.total_size;
    }
    return MEOW_HEAP_SIZE_BYTES; /* Return configured heap size as fallback */
}

/**
 * get_heap_used_size - Get used heap size (wrapper for new interface)
 */
static inline uint32_t get_heap_used_size(void) {
    cat_heap_stats_t stats = {0};
    if (meow_heap_get_stats(&stats) == MEOW_SUCCESS) {
        return stats.used_size;
    }
    return 0; /* Conservative fallback */
}

/**
 * get_heap_free_size - Get free heap size (wrapper for new interface)
 */
static inline uint32_t get_heap_free_size(void) {
    cat_heap_stats_t stats = {0};
    if (meow_heap_get_stats(&stats) == MEOW_SUCCESS) {
        return stats.free_size;
    }
    return MEOW_HEAP_SIZE_BYTES; /* Optimistic fallback */
}

/* ============================================================================
 * MEMORY STATISTICS
 * ============================================================================ */

void get_memory_statistics(memory_stats_t* stats) {
    if (!stats) {
        meow_log(MEOW_LOG_YOWL, "get_memory_statistics: NULL pointer");
        return;
    }

    if (!is_memory_management_initialized()) {
        meow_log(MEOW_LOG_HISS, "Memory management not initialized; returning zeros");
        stats->total_system_memory = 0;
        stats->available_memory = 0;
        stats->used_memory = 0;
        stats->heap_size = 0;
        stats->heap_used = 0;
        stats->heap_free = 0;
        stats->total_territories = 0;
        stats->safe_territories = 0;
        stats->occupied_territories = 0;
        return;
    }

    /* Physical memory totals from HAL */
    stats->total_system_memory = hal_memory_get_total_size();
    stats->available_memory    = hal_memory_get_available_size();
    stats->used_memory = stats->total_system_memory - stats->available_memory;

    /* Heap statistics from cat_heap (using new interface) */
    stats->heap_size = get_heap_total_size();
    stats->heap_used = get_heap_used_size();
    stats->heap_free = get_heap_free_size();

    /* Territory statistics from purr_memory */
    uint32_t total, occupied;
    get_purr_memory_stats(&total, &occupied, NULL);
    stats->total_territories = total;
    stats->occupied_territories = occupied;
    stats->safe_territories = total - occupied;
}

/* ============================================================================
 * MEMORY INITIALIZATION
 * ============================================================================ */

void init_cat_memory(multiboot_info_t* multiboot_info) {
    meow_log(MEOW_LOG_MEOW, "Starting cat memory management initialization...");

    /* Reset error state */
    last_error = MM_SUCCESS;

    /* Validate multiboot info */
    if (!multiboot_info) {
        meow_log(MEOW_LOG_YOWL, "Cannot initialize MM: null multiboot info!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }

    /* Additional validation for memory map */
    if (!(multiboot_info->flags & (1 << 6))) {
        meow_log(MEOW_LOG_YOWL, "Cannot initialize MM: no memory map available!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }

    if (!multiboot_info->mmap_addr || multiboot_info->mmap_length == 0) {
        meow_log(MEOW_LOG_YOWL, "Cannot initialize MM: invalid memory map data!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }

    meow_log(MEOW_LOG_CHIRP, "Memory map: %d bytes at 0x%x",
             multiboot_info->mmap_length, multiboot_info->mmap_addr);

    /* Step 1: Initialize territory mapping (safe with validated multiboot info) */
    meow_log(MEOW_LOG_MEOW, "Phase 1: Territory mapping...");
    initialize_territory_map(multiboot_info);

    /* Step 2: Initialize physical memory manager */
    meow_log(MEOW_LOG_MEOW, "Phase 2: Physical memory manager...");
    uint32_t total_memory = get_memory_size_from_territories();
    if (total_memory == 0) {
        meow_log(MEOW_LOG_YOWL, "No usable memory detected!");
        last_error = MM_ERROR_NO_MEMORY;
        return;
    }

    purr_memory_init(total_memory);

    /* Step 3: Initialize cat heap (using new interface) */
    meow_log(MEOW_LOG_MEOW, "Phase 3: Cat heap allocator...");
    meow_error_t heap_result = meow_heap_init();
    if (heap_result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "Failed to initialize heap: %s", 
                 meow_error_to_string(heap_result));
        last_error = MM_ERROR_HEAP_CORRUPTION;
        return;
    }

    /* Step 4: Run initial memory tests */
    meow_log(MEOW_LOG_MEOW, "Phase 4: Memory system validation...");
    if (!run_memory_validation_tests()) {
        meow_log(MEOW_LOG_YOWL, "Memory system validation failed!");
        last_error = MM_ERROR_HEAP_CORRUPTION;
        return;
    }

    /* Mark as initialized */
    mm_initialized = 1;
    last_error = MM_SUCCESS;

    /* Display memory summary */
    display_memory_summary();
    meow_log(MEOW_LOG_CHIRP, "Cat memory management fully initialized!");
}

/* ============================================================================
 * MEMORY VALIDATION TESTS
 * ============================================================================ */

static uint8_t run_memory_validation_tests(void) {
    meow_log(MEOW_LOG_MEOW, "Running memory validation tests...");

    /* Test 1: Basic heap allocation (using new interface) */
    void* test_ptr = meow_heap_alloc(64);
    if (!test_ptr) {
        meow_log(MEOW_LOG_YOWL, "Basic heap allocation test failed");
        return 0;
    }

    /* Test 2: Write and read test */
    uint8_t* test_bytes = (uint8_t*)test_ptr;
    for (int i = 0; i < 64; i++) {
        test_bytes[i] = (uint8_t)(i & 0xFF);
    }

    for (int i = 0; i < 64; i++) {
        if (test_bytes[i] != (uint8_t)(i & 0xFF)) {
            meow_log(MEOW_LOG_YOWL, "Memory read/write test failed at byte %d", i);
            meow_heap_free(test_ptr);
            return 0;
        }
    }

    /* Test 3: Free the allocation (using new interface) */
    if (meow_heap_free(test_ptr) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "Memory free test failed");
        return 0;
    }

    meow_log(MEOW_LOG_CHIRP, "Memory validation tests PASSED!");
    return 1;
}

/* ============================================================================
 * MEMORY STATUS DISPLAY
 * ============================================================================ */

void display_memory_summary(void) {
    if (!mm_initialized) {
        meow_log(MEOW_LOG_HISS, "Memory management not initialized yet!");
        return;
    }

    meow_printf("\n🐱 MEOWKERNEL MEMORY SUMMARY\n");
    meow_printf("============================\n");

    /* Get and display current statistics */
    get_memory_statistics(&current_stats);

    meow_printf("System Memory: %d MB\n", current_stats.total_system_memory / (1024 * 1024));
    meow_printf("Available: %d MB\n", current_stats.available_memory / (1024 * 1024));
    meow_printf("Heap Size: %d KB\n", current_stats.heap_size / 1024);
    meow_printf("Heap Used: %d KB\n", current_stats.heap_used / 1024);
    meow_printf("Territories: %d total, %d safe\n",
                current_stats.total_territories, current_stats.safe_territories);
    meow_printf("============================\n\n");
}

/* ============================================================================
 * MEMORY MANAGEMENT STATUS
 * ============================================================================ */

uint8_t is_memory_management_initialized(void) {
    return mm_initialized;
}

/* ============================================================================
 * ERROR HANDLING
 * ============================================================================ */

mm_error_t get_last_mm_error(void) {
    return last_error;
}

const char* get_mm_error_string(mm_error_t error) {
    switch (error) {
        case MM_SUCCESS:                    return "No error";
        case MM_ERROR_NOT_INITIALIZED:      return "Memory management not initialized";
        case MM_ERROR_NO_MEMORY:            return "No usable memory found";
        case MM_ERROR_INVALID_ADDRESS:      return "Invalid memory address";
        case MM_ERROR_INVALID_SIZE:         return "Invalid allocation size";
        case MM_ERROR_HEAP_CORRUPTION:      return "Heap corruption detected";
        case MM_ERROR_TERRITORY_INVALID:    return "Invalid memory territory";
        case MM_ERROR_MULTIBOOT_INVALID:    return "Invalid multiboot information";
        default:                            return "Unknown error";
    }
}