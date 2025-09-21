#include "meow_memory.h"
#include "territory_map.h"
#include "purr_memory.h"
#include "cat_heap.h"
#include "../../kernel/meow_util.h"
#include "../hal/hal.h"

// Global memory management state
static uint8_t mm_initialized = 0;
static memory_stats_t current_stats = {0};
static mm_error_t last_error = MM_SUCCESS;

static uint8_t run_memory_validation_tests(void);

void get_memory_statistics(memory_stats_t* stats) {
    if (!stats) {
        meow_error(" get_memory_statistics: NULL pointer");
        return;
    }
    if (!is_memory_management_initialized()) {
        meow_warn(" Memory management not initialized; returning zeros");
        stats->total_system_memory   = 0;
        stats->available_memory      = 0;
        stats->used_memory           = 0;
        stats->heap_size             = 0;
        stats->heap_used             = 0;
        stats->heap_free             = 0;
        stats->total_territories     = 0;
        stats->safe_territories      = 0;
        stats->occupied_territories  = 0;
        return;
    }

    // Physical memory totals from HAL
    stats->total_system_memory = hal_memory_get_total_size();
    stats->available_memory    = hal_memory_get_available_size();
    stats->used_memory         = stats->total_system_memory - stats->available_memory;

    // Heap statistics from cat_heap
    stats->heap_size = get_heap_total_size();
    stats->heap_used = get_heap_used_size();
    stats->heap_free = get_heap_free_size();

    // Territory statistics from purr_memory
    uint32_t total, occupied;
    get_purr_memory_stats(&total, &occupied, NULL);
    stats->total_territories    = total;
    stats->occupied_territories = occupied;
    stats->safe_territories     = total - occupied;
}

// FIXED: Safe memory management initialization with validation
void init_cat_memory(multiboot_info_t* multiboot_info) {
    meow_debug(" Starting cat memory management initialization...");
    
    // Reset error state
    last_error = MM_SUCCESS;
    
    // Validate multiboot info again
    if (!multiboot_info) {
        meow_error(" Cannot initialize MM: null multiboot info!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }
    
    // Additional validation for memory map
    if (!(multiboot_info->flags & (1 << 6))) {
        meow_error(" Cannot initialize MM: no memory map available!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }
    
    if (!multiboot_info->mmap_addr || multiboot_info->mmap_length == 0) {
        meow_error(" Cannot initialize MM: invalid memory map data!");
        last_error = MM_ERROR_MULTIBOOT_INVALID;
        return;
    }
    
    meow_info(" Memory map: %d bytes at 0x%x", 
              multiboot_info->mmap_length, multiboot_info->mmap_addr);
    
    // Step 1: Initialize territory mapping (safe with validated multiboot info)
    meow_debug(" Phase 1: Territory mapping...");
    initialize_territory_map(multiboot_info);  // Now safe to call
    
    // Step 2: Initialize physical memory manager
    meow_debug(" Phase 2: Physical memory manager...");
    uint32_t total_memory = get_memory_size_from_territories();
    if (total_memory == 0) {
        meow_error(" No usable memory detected!");
        last_error = MM_ERROR_NO_MEMORY;
        return;
    }
    
    purr_memory_init(total_memory);
    
    // Step 3: Initialize cat heap
    meow_debug("  Phase 3: Cat heap allocator...");
    setup_cat_heap();
    
    // Step 4: Run initial memory tests
    meow_debug(" Phase 4: Memory system validation...");
    if (!run_memory_validation_tests()) {
        meow_error(" Memory system validation failed!");
        last_error = MM_ERROR_HEAP_CORRUPTION;
        return;
    }
    
    // Mark as initialized
    mm_initialized = 1;
    last_error = MM_SUCCESS;
    
    // Display memory summary
    display_memory_summary();
    
    meow_info(" Cat memory management fully initialized!");
}

// Memory validation tests
static uint8_t run_memory_validation_tests(void) {
    meow_debug(" Running memory validation tests...");
    
    // Test 1: Basic heap allocation
    void* test_ptr = meow_alloc(64);
    if (!test_ptr) {
        meow_error(" Basic heap allocation test failed");
        return 0;
    }
    
    // Test 2: Write and read test
    uint8_t* test_bytes = (uint8_t*)test_ptr;
    for (int i = 0; i < 64; i++) {
        test_bytes[i] = (uint8_t)(i & 0xFF);
    }
    
    for (int i = 0; i < 64; i++) {
        if (test_bytes[i] != (uint8_t)(i & 0xFF)) {
            meow_error(" Memory read/write test failed at byte %d", i);
            meow_free(test_ptr);
            return 0;
        }
    }
    
    // Test 3: Free the allocation
    meow_free(test_ptr);
    
    meow_info(" Memory validation tests PASSED!");
    return 1;
}

// Safe memory status display
void display_memory_summary(void) {
    if (!mm_initialized) {
        meow_warn("  Memory management not initialized yet !!!");
        return;
    }
    
    meow_printf("\n MEOWKERNEL MEMORY SUMMARY\n");
    meow_printf("============================\n");
    
    // Get and display current statistics
    get_memory_statistics(&current_stats);
    
    meow_printf("System Memory: %d MB\n", current_stats.total_system_memory / (1024 * 1024));
    meow_printf("Available:     %d MB\n", current_stats.available_memory / (1024 * 1024));
    meow_printf("Heap Size:     %d KB\n", current_stats.heap_size / 1024);
    meow_printf("Heap Used:     %d KB\n", current_stats.heap_used / 1024);
    meow_printf("Territories:   %d total, %d safe\n", 
               current_stats.total_territories, current_stats.safe_territories);
    meow_printf("============================\n\n");
}

// Check if MM is properly initialized
uint8_t is_memory_management_initialized(void) {
    return mm_initialized;
}

// Error handling
mm_error_t get_last_mm_error(void) {
    return last_error;
}

const char* get_mm_error_string(mm_error_t error) {
    switch (error) {
        case MM_SUCCESS: return "No error";
        case MM_ERROR_NOT_INITIALIZED: return "Memory management not initialized";
        case MM_ERROR_NO_MEMORY: return "No usable memory found";
        case MM_ERROR_INVALID_ADDRESS: return "Invalid memory address";
        case MM_ERROR_INVALID_SIZE: return "Invalid allocation size";
        case MM_ERROR_HEAP_CORRUPTION: return "Heap corruption detected";
        case MM_ERROR_TERRITORY_INVALID: return "Invalid memory territory";
        case MM_ERROR_MULTIBOOT_INVALID: return "Invalid multiboot information";
        default: return "Unknown error";
    }
}

