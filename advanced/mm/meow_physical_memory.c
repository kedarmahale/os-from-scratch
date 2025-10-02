#include "meow_physical_memory.h"
#include "meow_memory_manager.h"
#include "../../kernel/meow_util.h"

// PMM Global State
static uint32_t total_territories = 0;
static uint32_t occupied_territories = 0;
static uint32_t* territory_bitmap = NULL;
static uint8_t pmm_initialized = 0;
static uint32_t bitmap_size_bytes = 0;
static uint32_t reserved_territories = 0;

void purr_memory_init(uint32_t memory_size) {
    meow_log(MEOW_LOG_CHIRP,"==== Purr Memory Manager initializing... ====");

    // SAFETY: Validate input parameters
    if (memory_size == 0) {
        meow_log(MEOW_LOG_YOWL," Cannot initialize PMM: zero memory size!!!!");
        return;
    }
    if (memory_size < (8 * 1024 * 1024)) { // Less than 8MB
        meow_log(MEOW_LOG_YOWL," Cannot initialize PMM: insufficient memory (%d bytes)", memory_size);
        return;
    }

    // Calculate territories (4KB pages)
    total_territories = memory_size / TERRITORY_SIZE;
    if (total_territories == 0) {
        meow_log(MEOW_LOG_YOWL," Cannot initialize PMM: zero territories calculated!");
        return;
    }
    if (total_territories > MAX_TERRITORIES) {
        meow_log(MEOW_LOG_HISS," Memory size too large, capping at %d territories", MAX_TERRITORIES);
        total_territories = MAX_TERRITORIES;
    }
    meow_log(MEOW_LOG_CHIRP," Total territories calculated: %d (memory: %d MB)",
              total_territories, memory_size / (1024 * 1024));

    // Calculate bitmap size
    bitmap_size_bytes = ((total_territories + 31) / 32) * sizeof(uint32_t);
    meow_log(MEOW_LOG_CHIRP,"Bitmap size needed: %d bytes (%d KB)",
              bitmap_size_bytes, bitmap_size_bytes / 1024);

    // Determine safe location for bitmap
    extern char _kernel_end;
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    uint32_t bitmap_start = (kernel_end + 0x1000 - 1) & ~(0x1000 - 1);  // Align to 4KB
    bitmap_start += 0x10000;  // Add 64KB safety margin
    territory_bitmap = (uint32_t*)bitmap_start;
    meow_log(MEOW_LOG_CHIRP," Kernel ends at: 0x%x", kernel_end);
    meow_log(MEOW_LOG_CHIRP," Bitmap placed at: 0x%x - 0x%x (%d bytes)",
              bitmap_start, bitmap_start + bitmap_size_bytes, bitmap_size_bytes);

    // SAFETY: Verify bitmap does not exceed physical RAM
    uint64_t end_addr = (uint64_t)bitmap_start + bitmap_size_bytes;
    if (end_addr > memory_size) {
        meow_log(MEOW_LOG_YOWL," Bitmap would extend beyond RAM! Start: 0x%x, Size: %d, RAM: %d",
                   bitmap_start, bitmap_size_bytes, memory_size);
        return;
    }

    // Calculate number of bitmap entries
    uint32_t bitmap_entries = (total_territories + 31) / 32;

    // Determine how many territories to reserve (all before bitmap)
    uint32_t first_free_addr = bitmap_start + bitmap_size_bytes;
    reserved_territories = first_free_addr / TERRITORY_SIZE;
    if (reserved_territories > total_territories) {
        reserved_territories = total_territories;
    }
    meow_log(MEOW_LOG_CHIRP," Reserving %u territories (addresses < 0x%x)",
              reserved_territories, first_free_addr);

    // Initialize bitmap: mark all as occupied
    for (uint32_t i = 0; i < bitmap_entries; i++) {
        territory_bitmap[i] = 0xFFFFFFFF;
    }
    occupied_territories = total_territories;

    // Mark territories after reserved region as free
    for (uint32_t t = reserved_territories; t < total_territories; t++) {
        uint32_t idx = t / 32;
        uint32_t bit = t % 32;
        territory_bitmap[idx] &= ~(1 << bit);
        occupied_territories--;
    }

    pmm_initialized = 1;
    meow_log(MEOW_LOG_CHIRP," Purr Memory Manager initialized successfully!");
    purr_status();
}

void purr_status(void) {
    if (!pmm_initialized) {
        meow_log(MEOW_LOG_HISS," PMM not initialized yet!!!!");
        return;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== PURR MEMORY MANAGER STATUS ====");
    meow_log(MEOW_LOG_CHIRP,"====================================");
    meow_log(MEOW_LOG_CHIRP,"Total territories: %d", total_territories);
    meow_log(MEOW_LOG_CHIRP,"Occupied territories: %d", occupied_territories);
    meow_log(MEOW_LOG_CHIRP,"Free territories: %d", total_territories - occupied_territories);
    meow_log(MEOW_LOG_CHIRP,"Bitmap location: 0x%x", (uint32_t)territory_bitmap);
    meow_log(MEOW_LOG_CHIRP,"Bitmap size: %d bytes", bitmap_size_bytes);
    meow_log(MEOW_LOG_CHIRP,"Memory utilization: %d%%", 
              total_territories > 0 ? (occupied_territories * 100 / total_territories) : 0);
    meow_log(MEOW_LOG_CHIRP,"====================================");
}

uint32_t purr_alloc_territory(void) {
    if (!pmm_initialized) {
        meow_log(MEOW_LOG_YOWL," Cannot allocate: PMM not initialized!!!!");
        return 0;
    }
    if (occupied_territories >= total_territories) {
        meow_log(MEOW_LOG_HISS," No free territories available!!!!");
        return 0;
    }

    // Start search from reserved_territories, not zero
    uint32_t bitmap_entries = (total_territories + 31) / 32;
    for (uint32_t t = reserved_territories; t < total_territories; t++) {
        uint32_t idx = t / 32;
        uint32_t bit = t % 32;

	// Only examine bits that were marked free
        if (!(territory_bitmap[idx] & (1 << bit))) {
            // Mark as occupied
            territory_bitmap[idx] |= (1 << bit);
            occupied_territories++;

            uint32_t physical_address = t * TERRITORY_SIZE;
            meow_log(MEOW_LOG_MEOW," Allocated territory %d (physical: 0x%x)", t, physical_address);
            return physical_address;
        }
    }

    meow_log(MEOW_LOG_HISS,"No free territories found past reserved region");
    return 0;
}

void purr_free_territory(uint32_t physical_address) {
    if (!pmm_initialized) {
        meow_log(MEOW_LOG_YOWL," Cannot free: PMM not initialized");
        return;
    }
    
    if (physical_address == 0) {
        meow_log(MEOW_LOG_HISS," Attempting to free NULL physical address");
        return;
    }
    
    uint32_t territory = physical_address / TERRITORY_SIZE;
    
    if (territory >= total_territories) {
        meow_log(MEOW_LOG_YOWL," Territory %d out of range (max: %d)", territory, total_territories - 1);
        return;
    }
    
    uint32_t bitmap_index = territory / 32;
    uint32_t bit_position = territory % 32;
    
    uint32_t bitmap_entries = (total_territories + 31) / 32;
    if (bitmap_index >= bitmap_entries) {
        meow_log(MEOW_LOG_YOWL," Bitmap index %d out of range (max: %d)", bitmap_index, bitmap_entries - 1);
        return;
    }
    
    // Check if already free
    if (!(territory_bitmap[bitmap_index] & (1 << bit_position))) {
        meow_log(MEOW_LOG_HISS," Territory %d already free", territory);
        return;
    }
    
    // Mark as free
    territory_bitmap[bitmap_index] &= ~(1 << bit_position);
    occupied_territories--;
    
    meow_log(MEOW_LOG_MEOW,"Freed territory %d (physical: 0x%x)", territory, physical_address);
}

uint8_t purr_memory_validate(void) {
    if (!pmm_initialized) {
        return 0;
    }
    
    // Basic sanity checks
    if (!territory_bitmap) {
        meow_log(MEOW_LOG_YOWL," PMM validation failed: NULL bitmap");
        return 0;
    }
    
    if (total_territories == 0) {
        meow_log(MEOW_LOG_YOWL," PMM validation failed: zero territories");
        return 0;
    }
    
    if (occupied_territories > total_territories) {
        meow_log(MEOW_LOG_YOWL," PMM validation failed: occupied > total");
        return 0;
    }
    
    meow_log(MEOW_LOG_MEOW," PMM validation passed!!!\n");
    return 1;
}

void get_purr_memory_stats(uint32_t* total, uint32_t* occupied, uint32_t* free) {
    if (total) *total = pmm_initialized ? total_territories : 0;
    if (occupied) *occupied = pmm_initialized ? occupied_territories : 0;
    if (free) *free = pmm_initialized ? (total_territories - occupied_territories) : 0;
}

uint8_t is_purr_memory_initialized(void) {
    return pmm_initialized;
}

