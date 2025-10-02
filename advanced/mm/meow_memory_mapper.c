#include "meow_memory_mapper.h"
#include "meow_memory_manager.h"
#include "../hal/meow_hal_interface.h"
#include "../../kernel/meow_util.h"

#define KERNEL_START 0x100000      // 1MB - where kernel cats live

// Global territory database (cat's mental map)
static cat_territory_info_t cat_territories[MAX_TERRITORIES];
static uint32_t territory_count = 0;
static uint64_t total_available_memory = 0;
static cat_territory_info_t* largest_safe_territory = NULL;

// Initialize the complete territory mapping system
void initialize_territory_map(multiboot_info_t* mbi) {
    meow_log(MEOW_LOG_MEOW,"  Cats are exploring and mapping territories...");
    
    // Reset territory database
    territory_count = 0;
    total_available_memory = 0;
    largest_safe_territory = NULL;
    
    // Parse multiboot memory map provided by GRUB
    parse_multiboot_territories(mbi);
    
    // Detect and classify cat territories
    detect_cat_territories();
    
    // Set up safe boundaries
    setup_territory_boundaries();
    
    // Mark kernel's special territory
    mark_kernel_territory();
    
    // Set up reserved areas for special cat activities
    setup_reserved_cat_areas();
    
    // Display the complete territory map
    print_territory_map();
    
    meow_log(MEOW_LOG_MEOW,"  Territory mapping complete - cats know their domain!");
}

// Parse multiboot memory map into cat territories
void parse_multiboot_territories(multiboot_info_t* mbi) {
    meow_log(MEOW_LOG_MEOW," Parsing GRUB's territory reports...");
    
    // Check if multiboot info has memory map
    if (!(mbi->flags & (1 << 6))) {
        meow_log(MEOW_LOG_MEOW," No memory map from GRUB - cats are confused!");
        return;
    }
    
    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)mbi->mmap_addr;
    
    meow_printf(" GRUB found %d bytes of territory info\n", mbi->mmap_length);
    
    // Parse each memory region
    while ((uint32_t)mmap < mbi->mmap_addr + mbi->mmap_length) {
        if (territory_count >= MAX_TERRITORIES) {
            meow_log(MEOW_LOG_MEOW," Too many territories - cats are overwhelmed!");
            break;
        }
        
        cat_territory_info_t* territory = &cat_territories[territory_count];
        
        territory->start_addr = mmap->addr;
        territory->size = mmap->len;
        territory->type = mmap->type;
        
        // Classify territory based on type
        switch (mmap->type) {
            case TERRITORY_TYPE_AVAILABLE:
                territory->safe_for_cats = 1;
                territory->cat_description = "Safe cat territory";
                total_available_memory += mmap->len;
                
                // Check if this is the largest safe territory
                if (!largest_safe_territory || mmap->len > largest_safe_territory->size) {
                    largest_safe_territory = territory;
                }
                break;
                
            case TERRITORY_TYPE_RESERVED:
                territory->safe_for_cats = 0;
                territory->cat_description = "Dangerous - cats avoid";
                break;
                
            case TERRITORY_TYPE_ACPI_RECLAIM:
                territory->safe_for_cats = 1; // Can be reclaimed later
                territory->cat_description = "Special cat zone (reclaimable)";
                break;
                
            case TERRITORY_TYPE_ACPI_NVS:
                territory->safe_for_cats = 0;
                territory->cat_description = "Hardware area - cats stay away";
                break;
                
            default:
                territory->safe_for_cats = 0;
                territory->cat_description = "Unknown territory - cats avoid";
                break;
        }
        
        meow_printf("  Territory %d: 0x%llx - 0x%llx (%s)\n",
                   territory_count,
                   territory->start_addr,
                   territory->start_addr + territory->size - 1,
                   territory->cat_description);
        
        territory_count++;
        
        // Move to next entry
        mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    meow_printf(" Found %d territories, %llu bytes safe for cats\n", 
               territory_count, total_available_memory);
}

// Detect and classify available cat territories
void detect_cat_territories(void) {
    meow_log(MEOW_LOG_MEOW," Cats are investigating territory safety...");
    
    uint32_t safe_territories = 0;
    
    for (uint32_t i = 0; i < territory_count; i++) {
        cat_territory_info_t* territory = &cat_territories[i];
        
        // Additional safety validation
        if (territory->safe_for_cats) {
            if (validate_territory_safety(territory->start_addr, territory->size)) {
                safe_territories++;
                meow_printf(" Territory %d verified safe for cats\n", i);
            } else {
                territory->safe_for_cats = 0;
                territory->cat_description = "Failed safety check - cats avoid";
                meow_printf(" Territory %d failed safety check\n", i);
            }
        }
    }
    
    meow_printf(" %d territories confirmed safe for cat activities\n", safe_territories);
}

// Set up safe boundaries for cat territories
void setup_territory_boundaries(void) {
    meow_log(MEOW_LOG_MEOW," Setting up territory boundaries...");
    
    // Ensure kernel territory is properly marked
    for (uint32_t i = 0; i < territory_count; i++) {
        cat_territory_info_t* territory = &cat_territories[i];
        
        // Check if territory overlaps with kernel
        if (territory->start_addr <= KERNEL_START && 
            territory->start_addr + territory->size > KERNEL_START) {
            
            meow_printf(" Territory %d contains kernel cats' home\n", i);
            
            // Ensure kernel area is not used for general allocation
            if (territory->start_addr + territory->size > KERNEL_START + KERNEL_SIZE) {
                meow_printf(" Protecting kernel territory from other cats\n");
            }
        }
    }
}

// Ensure territories are safe for cats to use
uint8_t validate_territory_safety(uint64_t addr, uint64_t size) {
    // Basic safety checks
    
    // 1. Territory must be above 1MB (cats avoid low memory chaos)
    if (addr < 0x100000) {
        return 0; // Too low - dangerous for cats
    }
    
    // 2. Territory must be properly aligned (cats like order)
    if (addr & 0xFFF) {
        return 0; // Not page-aligned - cats are particular
    }
    
    // 3. Territory must be reasonable size (not too small for cats)
    if (size < 4096) {
        return 0; // Too small - cats need space
    }
    
    // 4. Territory must not be too high (cats avoid dangerous heights)
    if (addr > 0xFFFFFFFF) {
        return 0; // Too high - cats prefer lower addresses for now
    }
    
    return 1; // Territory is safe for cats
}

// Find the biggest territory for main cat activities
cat_territory_info_t* get_largest_territory(void) {
    return largest_safe_territory;
}

// Mark where the kernel cats live (reserved)
void mark_kernel_territory(void) {
    meow_log(MEOW_LOG_MEOW," Marking kernel cats' home territory...");
    
    meow_printf(" Kernel territory: 0x%x - 0x%x (size: %d KB)\n",
               KERNEL_START,
               KERNEL_START + KERNEL_SIZE - 1,
               KERNEL_SIZE / 1024);
    
    // Kernel territory is where the main cats live and work
    // This area should not be used for general memory allocation
}

// Display territory map for debugging
void print_territory_map(void) {
    meow_printf("\n  CAT TERRITORY MAP\n");
    meow_printf("=====================================\n");
    
    for (uint32_t i = 0; i < territory_count; i++) {
        cat_territory_info_t* territory = &cat_territories[i];
        
        const char* safety_icon = territory->safe_for_cats ? "Y" : "N";
        
        meow_printf("%s Territory %2d: 0x%08llx - 0x%08llx (%8llu KB) - %s\n",
                   safety_icon,
                   i,
                   territory->start_addr,
                   territory->start_addr + territory->size - 1,
                   territory->size / 1024,
                   territory->cat_description);
    }
    
    meow_printf("=====================================\n");
    meow_printf(" Total available: %llu MB (%llu KB)\n",
               total_available_memory / (1024 * 1024),
               total_available_memory / 1024);
    
    if (largest_safe_territory) {
        meow_printf(" Largest territory: 0x%08llx (%llu MB)\n",
                   largest_safe_territory->start_addr,
                   largest_safe_territory->size / (1024 * 1024));
    }
    
    meow_printf(" %d territories ready for cat activities!\n\n", territory_count);
}

// Calculate total available territory size
uint32_t get_memory_size_from_territories(void) {
    return (uint32_t)total_available_memory;
}

// Reserve special areas for important cat activities
void setup_reserved_cat_areas(void) {
    meow_log(MEOW_LOG_MEOW," Setting up special cat activity areas...");
    
    // Reserve area for territory bitmap (cats need to track their domain)
    meow_printf(" Territory bitmap area: 0x100000 - 0x108000 (32KB)\n");
    
    // Reserve area for cat heap (cozy sleeping areas)
    meow_printf("  Cat heap area: 0x200000 - 0x300000 (1MB)\n");
    
    // These areas are managed by the purr memory manager
    meow_log(MEOW_LOG_MEOW," Special cat areas reserved and ready!");
}

// Query functions for other parts of the kernel

// Check if an address is in safe cat territory
uint8_t is_safe_cat_territory(uint64_t addr) {
    for (uint32_t i = 0; i < territory_count; i++) {
        cat_territory_info_t* territory = &cat_territories[i];
        
        if (territory->safe_for_cats &&
            addr >= territory->start_addr &&
            addr < territory->start_addr + territory->size) {
            return 1;
        }
    }
    return 0;
}

// Get total number of safe territories
uint32_t get_total_safe_territories(void) {
    uint32_t safe_count = 0;
    
    for (uint32_t i = 0; i < territory_count; i++) {
        if (cat_territories[i].safe_for_cats) {
            safe_count++;
        }
    }
    
    return safe_count;
}

// Get total size of available territories
uint64_t get_available_territory_size(void) {
    return total_available_memory;
}
