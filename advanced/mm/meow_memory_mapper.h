/* advanced/mm/meow_memory_mapper.h - MeowKernel Memory Mapper Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_MEMORY_MAPPER_H
#define MEOW_MEMORY_MAPPER_H

#include <stdint.h>
#include <stddef.h>
#include "../kernel/meow_multiboot.h" /* Multiboot info structure */

// =============================================================================
// MEMORY REGION TYPES
// =============================================================================

// Memory region types (cat territory classification)
#define TERRITORY_TYPE_AVAILABLE    1  // Safe cat territories
#define TERRITORY_TYPE_RESERVED     2  // Dangerous - cats avoid
#define TERRITORY_TYPE_ACPI_RECLAIM 3  // Special cat zones
#define TERRITORY_TYPE_ACPI_NVS     4  // Cats stay away

// =============================================================================
// CAT TERRITORY STRUCTURES
// =============================================================================

// Cat territory information structure
typedef struct cat_territory_info {
    uint64_t start_addr;
    uint64_t size;
    uint32_t type;
    uint8_t safe_for_cats;
    const char* cat_description;
} cat_territory_info_t;

// =============================================================================
// TERRITORY MAP FUNCTION DECLARATIONS
// =============================================================================

// Territory map functions
void initialize_territory_map(multiboot_info_t* mbi);
void parse_multiboot_territories(multiboot_info_t* mbi);
void detect_cat_territories(void);
void setup_territory_boundaries(void);
uint8_t validate_territory_safety(uint64_t addr, uint64_t size);
cat_territory_info_t* get_largest_territory(void);
void mark_kernel_territory(void);
void print_territory_map(void);
uint32_t get_memory_size_from_territories(void);
void setup_reserved_cat_areas(void);

// Territory query functions
uint8_t is_safe_cat_territory(uint64_t addr);
uint32_t get_total_safe_territories(void);
uint64_t get_available_territory_size(void);

// Territory management functions
cat_territory_info_t* get_territory_by_address(uint64_t addr);
uint32_t get_territory_count(void);
void validate_all_territories(void);

#endif // MEOW_MEMORY_MAPPER_H

