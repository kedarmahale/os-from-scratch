/* advanced/mm/meow_physical_memory.c - MeowKernel Physical Memory Manager
 *                                      Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_PHYSICAL_MEMORY_H
#define MEOW_PHYSICAL_MEMORY_H

#include <stdint.h>
#include <stddef.h>

// Constants for physical memory management
#define TERRITORY_SIZE 4096         // 4KB territories (like cat territories)
#define MAX_TERRITORIES 32768       // Support up to 128MB of cat territories

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

// Initialize the Purr Memory Manager
void purr_memory_init(uint32_t memory_size);

// Allocate a territory (like a cat claiming a spot)
uint32_t purr_alloc_territory(void);

// Free a territory (cat abandons a spot)  
void purr_free_territory(uint32_t);

// Display purr status (how content our memory manager is)
void purr_status(void);

// Query functions
uint32_t get_total_territories(void);
uint32_t get_free_territories(void);
uint32_t get_used_territories(void);

// Debug and information functions
void purr_debug_info(void);
void print_territory_bitmap(void);

// Memory validation functions
uint8_t is_valid_territory(void* territory);
uint8_t is_territory_free(void* territory);

void get_purr_memory_stats(uint32_t* total, uint32_t* occupied, uint32_t* free);

#endif // MEOW_PHYSICAL_MEMORY_H

