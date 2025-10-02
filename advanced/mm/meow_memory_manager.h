/* advanced/mm/meow_memory_manager.h - MeowKernel Memory Management interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_MEMORY_MANAGER_H
#define MEOW_MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "meow_physical_memory.h"    // Include for PMM functions
#include "meow_heap_allocator.h"     // Include for heap functions

// =============================================================================
// MEMORY MANAGEMENT CONFIGURATION
// =============================================================================

// Memory layout constants
#define KERNEL_START_ADDR       0x100000    // 1MB - where kernel cats live
#define KERNEL_SIZE             0x100000    // 1MB kernel size
#define TERRITORY_BITMAP_ADDR   0x100000    // Territory tracking
#define CAT_HEAP_BASE           0x200000    // Cat heap base address

// Memory management flags
#define MM_FLAG_INITIALIZED     0x01
#define MM_FLAG_HEAP_READY      0x02
#define MM_FLAG_PMM_READY       0x04
#define MM_FLAG_TERRITORIES_MAPPED 0x08

// Memory management statistics
typedef struct memory_stats {
    uint32_t total_system_memory;
    uint32_t available_memory;
    uint32_t used_memory;
    uint32_t heap_size;
    uint32_t heap_used;
    uint32_t heap_free;
    uint32_t total_territories;
    uint32_t safe_territories;
    uint32_t occupied_territories;
} memory_stats_t;

// =============================================================================
// MAIN MEMORY MANAGEMENT FUNCTIONS
// =============================================================================

// Master initialization function (coordinates all MM components)
void init_cat_memory(multiboot_info_t* multiboot_info);

// Memory management status and information
uint8_t is_memory_management_initialized(void);
void display_memory_summary(void);
void run_memory_tests(void);
void get_memory_statistics(memory_stats_t* stats);

// Unified memory allocation interface (uses heap allocator)
void* kmalloc(size_t size);     // Alias for meow_alloc
void kfree(void* ptr);          // Alias for meow_free

void get_memory_statistics(memory_stats_t* stats);

// =============================================================================
// MEMORY VALIDATION AND DEBUG FUNCTIONS
// =============================================================================

// Memory validation
uint8_t validate_memory_address(void* addr);
uint8_t validate_memory_range(void* start, size_t size);

// Debug and diagnostic functions
void dump_memory_layout(void);
void print_memory_map(void);
void test_memory_allocation(void);
void test_territory_system(void);

// Memory management configuration
void set_memory_debug_level(uint8_t level);
uint8_t get_memory_debug_level(void);

// =============================================================================
// ERROR HANDLING
// =============================================================================

// Memory management error codes
typedef enum {
    MM_SUCCESS = 0,
    MM_ERROR_NOT_INITIALIZED,
    MM_ERROR_NO_MEMORY,
    MM_ERROR_INVALID_ADDRESS,
    MM_ERROR_INVALID_SIZE,
    MM_ERROR_HEAP_CORRUPTION,
    MM_ERROR_TERRITORY_INVALID,
    MM_ERROR_MULTIBOOT_INVALID
} mm_error_t;

// Error handling functions
mm_error_t get_last_mm_error(void);
const char* get_mm_error_string(mm_error_t error);
void clear_mm_error(void);

#endif // MEOW_MEMORY_MANAGER_H
