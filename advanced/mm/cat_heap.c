#include "cat_heap.h"
#include "meow_memory.h"
#include "../../kernel/meow_util.h"

// Cat heap globals
static cat_memory_block_t* first_cat_bed = NULL;
static uint32_t heap_total_size = 0;
static uint32_t heap_used_size = 0;
static uint32_t heap_free_size = 0;
static uint32_t heap_block_count = 0;
static uint8_t heap_initialized = 0;

// Set up the cat heap (prepare cozy sleeping areas)
void setup_cat_heap(void) {
    first_cat_bed = (cat_memory_block_t*)CAT_HEAP_START;
    first_cat_bed->size = CAT_HEAP_SIZE - sizeof(cat_memory_block_t);
    first_cat_bed->occupied = 0;  // No cat sleeping yet
    first_cat_bed->next_bed = NULL;
    
    // Initialize heap statistics
    heap_total_size = CAT_HEAP_SIZE;
    heap_used_size = sizeof(cat_memory_block_t); // Header block
    heap_free_size = first_cat_bed->size;
    heap_block_count = 1;
    heap_initialized = 1;
    
    meow_debug(" Cat heap setup complete - beds ready!");
    meow_printf("  Cat Heap: %d bytes of cozy space at 0x%x\n", 
               CAT_HEAP_SIZE, CAT_HEAP_START);
}

// Allocate memory for cats (give them a cozy space)
void* meow_alloc(size_t size) {
    if (!heap_initialized) {
        setup_cat_heap();
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // Align size to 4-byte boundary (cats like neat arrangements)
    size = (size + 3) & ~3;
    
    cat_memory_block_t* current_bed = first_cat_bed;
    
    while (current_bed) {
        // Find an unoccupied bed big enough for our cat
        if (!current_bed->occupied && current_bed->size >= size) {
            
            // Split the bed if it's too big (cats like appropriately sized spaces)
            if (current_bed->size > size + sizeof(cat_memory_block_t) + 16) {
                cat_memory_block_t* new_bed = (cat_memory_block_t*)
                    ((uint8_t*)current_bed + sizeof(cat_memory_block_t) + size);
                
                new_bed->size = current_bed->size - size - sizeof(cat_memory_block_t);
                new_bed->occupied = 0;  // Empty bed
                new_bed->next_bed = current_bed->next_bed;
                
                current_bed->next_bed = new_bed;
                current_bed->size = size;
                
                heap_block_count++;
            }
            
            current_bed->occupied = 1;  // Cat is now sleeping here
            
            // Update heap statistics
            heap_used_size += size + sizeof(cat_memory_block_t);
            heap_free_size -= size + sizeof(cat_memory_block_t);
            
            void* cat_space = (void*)((uint8_t*)current_bed + sizeof(cat_memory_block_t));
            meow_debug("😴 Cat found cozy space at 0x%x (%d bytes)", 
                       (uint32_t)cat_space, size);
            
            return cat_space;
        }
        current_bed = current_bed->next_bed;
    }
    
    meow_debug("😿 No cozy spaces available for cat!");
    return NULL; // No space in the cat heap
}

// Free memory (cat leaves their cozy space)
void meow_free(void* ptr) {
    if (!ptr || !heap_initialized) {
        return;
    }
    
    cat_memory_block_t* cat_bed = (cat_memory_block_t*)
        ((uint8_t*)ptr - sizeof(cat_memory_block_t));
    
    if (!cat_bed->occupied) {
        meow_debug("⚠️  Attempting to free already free cat bed!");
        return;
    }
    
    cat_bed->occupied = 0;  // Cat has left the bed
    
    // Update heap statistics
    heap_used_size -= cat_bed->size + sizeof(cat_memory_block_t);
    heap_free_size += cat_bed->size + sizeof(cat_memory_block_t);
    
    meow_debug("🚶 Cat left their space at 0x%x", (uint32_t)ptr);
    
    // Try to merge with next block (cats like spacious areas)
    if (cat_bed->next_bed && !cat_bed->next_bed->occupied) {
        cat_bed->size += cat_bed->next_bed->size + sizeof(cat_memory_block_t);
        cat_bed->next_bed = cat_bed->next_bed->next_bed;
        heap_block_count--;
        meow_debug("  Merged cat beds for more space!");
    }
    
    // Try to merge with previous block (more complex, simplified for now)
    merge_free_blocks();
}

// Heap information functions
uint32_t get_heap_total_size(void) {
    return heap_total_size;
}

uint32_t get_heap_used_size(void) {
    return heap_used_size;
}

uint32_t get_heap_free_size(void) {
    return heap_free_size;
}

uint32_t get_heap_block_count(void) {
    return heap_block_count;
}

// Cat heap status display
void cat_heap_status(void) {
    if (!heap_initialized) {
        meow_printf(" Cat heap not initialized yet\n");
        return;
    }
    
    meow_printf("\n  CAT HEAP STATUS\n");
    meow_printf("==================\n");
    meow_printf("Total Size:    %d bytes (%d KB)\n", heap_total_size, heap_total_size / 1024);
    meow_printf("Used Size:     %d bytes (%d KB)\n", heap_used_size, heap_used_size / 1024);
    meow_printf("Free Size:     %d bytes (%d KB)\n", heap_free_size, heap_free_size / 1024);
    meow_printf("Block Count:   %d blocks\n", heap_block_count);
    meow_printf("Utilization:   %d%%\n", (heap_used_size * 100) / heap_total_size);
    meow_printf("Heap Start:    0x%x\n", CAT_HEAP_START);
    meow_printf("First Bed:     0x%x\n", (uint32_t)first_cat_bed);
    meow_printf("==================\n\n");
}

// Print detailed heap block information
void print_heap_blocks(void) {
    if (!heap_initialized) {
        meow_printf(" Cat heap not initialized\n");
        return;
    }
    
    meow_printf("\n CAT BED LAYOUT\n");
    meow_printf("================\n");
    
    cat_memory_block_t* current = first_cat_bed;
    uint32_t block_num = 0;
    
    while (current) {
        const char* status = current->occupied ? " OCCUPIED" : " FREE";
        meow_printf("Bed %2d: 0x%08x | Size: %6d bytes | %s\n",
                   block_num,
                   (uint32_t)current,
                   current->size,
                   status);
        
        current = current->next_bed;
        block_num++;
    }
    
    meow_printf("================\n\n");
}

// Validate heap integrity
void validate_heap_integrity(void) {
    if (!heap_initialized) {
        meow_debug("Heap not initialized - skipping validation");
        return;
    }
    
    meow_debug("🔍 Validating heap integrity...");
    
    cat_memory_block_t* current = first_cat_bed;
    uint32_t blocks_found = 0;
    uint32_t total_size_found = 0;
    
    while (current) {
        blocks_found++;
        total_size_found += current->size + sizeof(cat_memory_block_t);
        
        // Check for corruption
        if (current->size == 0) {
            meow_debug(" Found zero-sized block at 0x%x", (uint32_t)current);
            return;
        }
        
        if (current->size > CAT_HEAP_SIZE) {
            meow_debug(" Found oversized block at 0x%x", (uint32_t)current);
            return;
        }
        
        current = current->next_bed;
    }
    
    if (blocks_found != heap_block_count) {
        meow_debug(" Block count mismatch: found %d, expected %d", 
                   blocks_found, heap_block_count);
        return;
    }
    
    meow_debug(" Heap integrity validated: %d blocks, %d bytes", 
               blocks_found, total_size_found);
}

// Defragment cat heap (merge adjacent free blocks)
void defragment_cat_heap(void) {
    if (!heap_initialized) {
        return;
    }
    
    meow_debug(" Defragmenting cat heap...");
    
    cat_memory_block_t* current = first_cat_bed;
    uint32_t merges = 0;
    
    while (current && current->next_bed) {
        if (!current->occupied && !current->next_bed->occupied) {
            // Merge with next block
            current->size += current->next_bed->size + sizeof(cat_memory_block_t);
            current->next_bed = current->next_bed->next_bed;
            heap_block_count--;
            merges++;
        } else {
            current = current->next_bed;
        }
    }
    
    meow_debug(" Defragmentation complete: %d merges performed", merges);
}

// Find free block of specified size
cat_memory_block_t* find_free_block(size_t size) {
    cat_memory_block_t* current = first_cat_bed;
    
    while (current) {
        if (!current->occupied && current->size >= size) {
            return current;
        }
        current = current->next_bed;
    }
    
    return NULL;
}

// Merge adjacent free blocks
void merge_free_blocks(void) {
    if (!heap_initialized) {
        return;
    }
    
    cat_memory_block_t* current = first_cat_bed;
    
    while (current && current->next_bed) {
        if (!current->occupied && !current->next_bed->occupied) {
            // Check if blocks are adjacent
            uint8_t* current_end = (uint8_t*)current + sizeof(cat_memory_block_t) + current->size;
            uint8_t* next_start = (uint8_t*)current->next_bed;
            
            if (current_end == next_start) {
                // Blocks are adjacent, merge them
                current->size += current->next_bed->size + sizeof(cat_memory_block_t);
                current->next_bed = current->next_bed->next_bed;
                heap_block_count--;
                continue; // Check again with same current block
            }
        }
        current = current->next_bed;
    }
}

// Advanced allocation functions
void* meow_realloc(void* ptr, size_t new_size) {
    if (!ptr) {
        return meow_alloc(new_size);
    }
    
    if (new_size == 0) {
        meow_free(ptr);
        return NULL;
    }
    
    cat_memory_block_t* block = (cat_memory_block_t*)
        ((uint8_t*)ptr - sizeof(cat_memory_block_t));
    
    if (block->size >= new_size) {
        return ptr; // Current block is large enough
    }
    
    // Need larger block
    void* new_ptr = meow_alloc(new_size);
    if (new_ptr) {
        // Copy old data
        uint8_t* src = (uint8_t*)ptr;
        uint8_t* dst = (uint8_t*)new_ptr;
        for (uint32_t i = 0; i < block->size && i < new_size; i++) {
            dst[i] = src[i];
        }
        meow_free(ptr);
    }
    
    return new_ptr;
}

void* meow_calloc(size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = meow_alloc(total_size);
    
    if (ptr) {
        // Zero the memory
        uint8_t* bytes = (uint8_t*)ptr;
        for (size_t i = 0; i < total_size; i++) {
            bytes[i] = 0;
        }
    }
    
    return ptr;
}

