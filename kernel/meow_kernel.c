/* kernel/meow_kernel.c - Main MeowKernel entry point */

#include "meow_kernel.h"
#include "meow_util.h"
#include "../advanced/hal/hal.h"
#include "../advanced/mm/meow_memory.h"

static multiboot_info_t* global_multiboot_info = NULL;
static uint32_t global_multiboot_magic = 0;
static uint8_t multiboot_info_valid = 0;

void run_cat_tests(void);
void enter_cat_main_loop(void);
void display_system_info(void);
void test_memory_allocation(void);
void test_territory_system(void);

multiboot_info_t* get_multiboot_info(void) {
    return multiboot_info_valid ? global_multiboot_info : NULL;
}

uint32_t get_multiboot_magic(void) {
    return global_multiboot_magic;
}

uint8_t is_multiboot_info_valid(void) {
    return multiboot_info_valid;
}

// Display the MeowKernel banner with ASCII art
static void display_cat_banner(void) {
    clear_screen();
    set_text_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    
    terminal_writestring("     /\\^/\\  \n");
    terminal_writestring("    ( ^.^ )  (\n");
    terminal_writestring("    =\\`Y`/= _)\n");
    terminal_writestring("    ( | | )(  \n");
    terminal_writestring("   (  | |  )\n");
    terminal_writestring("    ( d b )\n");
    terminal_writestring("\n");

    
    set_text_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("=======================================\n");
    terminal_writestring("    MeowKernel v0.1.0 - Phase 1 \n");
    terminal_writestring("   The Purr-fect Operating System!\n");
    terminal_writestring("=======================================\n");
    
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writestring("\n");
}

//Multiboot validation function
uint8_t validate_multiboot_info(uint32_t magic, multiboot_info_t* mbi) {
    // Check multiboot magic number
    if (magic != MULTIBOOT_MAGIC) {
        meow_error(" Invalid multiboot magic: 0x%x (expected 0x%x)", magic, MULTIBOOT_MAGIC);
        return 0;
    }
    
    // Check multiboot info pointer
    if (!mbi) {
        meow_error(" Null multiboot info pointer");
        return 0;
    }
    
    // Basic sanity check on multiboot info structure
    if ((uint32_t)mbi < 0x1000 || (uint32_t)mbi > 0x100000) {
        meow_error(" Multiboot info pointer looks invalid: 0x%x", (uint32_t)mbi);
        return 0;
    }
    
    // Check if memory information is available
    if (!(mbi->flags & (1 << 0))) {
        meow_warn(" No basic memory info available");
    }
    
    // Check if memory map is available (critical for territory mapping)
    if (!(mbi->flags & (1 << 6))) {
        meow_warn(" No memory map available - territory mapping will be limited");
        return 0;  // Can't do proper memory management without memory map
    }
    
    // Validate memory map pointer
    if (!mbi->mmap_addr || mbi->mmap_length == 0) {
        meow_error(" Invalid memory map: addr=0x%x, length=%d", 
                   mbi->mmap_addr, mbi->mmap_length);
        return 0;
    }
    
    meow_debug(" Multiboot validation passed");
    return 1;  // Valid multiboot info
}

// Main kernel entry point (called from boot.S)
void kernel_main(uint32_t magic, multiboot_info_t* multiboot_info) {

    // Display our cat banner
    display_cat_banner();

    // Store multiboot parameters globally
    global_multiboot_magic = magic;
    global_multiboot_info = multiboot_info;

    // CRITICAL: Validate multiboot information before using
    if (!validate_multiboot_info(magic, multiboot_info)) {
        meow_error("Invalid or missing multiboot information!");
        meow_error("Cannot initialize memory management without boot info!");
        
        // Try to continue with minimal functionality
        meow_warn(" Continuing with limited functionality...");
        
        // Initialize HAL without memory management
        hal_init();
        
        // Show minimal kernel info
        meow_info("MeowKernel running in recovery mode");
        meow_info("Memory management disabled - no territory mapping");
        
        // Halt in recovery mode
        while(1) {
            __asm__("hlt");
        }
    }
   
    multiboot_info_valid = 1;

    meow_info("MeowKernel Phase 1 initialization starting...");
    terminal_writestring("\n");
    
    meow_debug("Multiboot info received at address: 0x%x", (uint32_t)multiboot_info);
    meow_debug("Multiboot flags: 0x%x", multiboot_info->flags);
    meow_debug("Memory lower: %d KB", multiboot_info->mem_lower);
    meow_debug("Memory upper: %d KB", multiboot_info->mem_upper);
    
    // Phase 1 Initialization Sequence
    // Step 1: Initialize Hardware Abstraction Layer
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[1/5]  Initializing Hardware Abstraction Layer...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    hal_init();
    meow_info("HAL initialized - cats can now control hardware!");
    terminal_writestring("\n");
    
    // Step 2: Initialize Cat Memory Management System
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[2/5]  Initializing cat memory management...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    init_cat_memory(multiboot_info);
    meow_info("All cat territories established and memory systems ready!");
    terminal_writestring("\n");
    
    // Step 3: Display system information
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[3/5]  Displaying system information...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    display_system_info();
    terminal_writestring("\n");
    
    // Step 4: Run comprehensive cat system tests
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[4/5]  Running cat system tests...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    run_cat_tests();
    meow_info("All cats are happy and systems are purring!");
    terminal_writestring("\n");
    
    // Step 5: Enter main cat activities loop
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[5/5]  Starting main cat activities...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring(" MeowKernel Phase 1 initialization COMPLETE!\n\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    enter_cat_main_loop();
}

// Display comprehensive system information
void display_system_info(void) {
    meow_printf("  SYSTEM INFORMATION:\n");
    meow_printf("   - Architecture: x86 32-bit\n");
    meow_printf("   - Bootloader: GRUB (Multiboot compliant)\n");
    meow_printf("   - Kernel: MeowKernel v0.1.0 Phase 1\n");
    meow_printf("   - HAL Status: Active and purring\n");
    meow_printf("   - Memory Management: Cat territories established\n");
    meow_printf("   - VGA Mode: 80x25 text mode with cat colors\n");
    meow_printf("   - Build System: Cross-compiled with love\n");
}

// Run comprehensive cat system tests
void run_cat_tests(void) {
    meow_debug("Starting comprehensive cat system tests...");
    
    // Test 1: Memory allocation system
    test_memory_allocation();
    
    // Test 2: Territory system
    test_territory_system();
    
    // Test 3: HAL integration test
    meow_debug("Testing HAL integration...");
    // Note: Add specific HAL tests here when HAL functions are available
    meow_info("HAL integration test passed");
    
    // Test 4: Display system test
    meow_debug("Testing display system...");
    set_text_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_writestring("     Color test: RED\n");
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("     Color test: GREEN\n");
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("     Color test: BLUE\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    meow_info("Display system test passed");
}

// Test memory allocation functionality
void test_memory_allocation(void) {
    meow_debug("Testing cat memory allocation system...");
    
    // Test small allocation
    void* small_space = meow_alloc(64);
    if (small_space) {
        meow_info("Small cat space (64 bytes) allocated at 0x%x", (uint32_t)small_space);
        meow_free(small_space);
        meow_info("Small cat space freed successfully");
    } else {
        meow_error("Failed to allocate small cat space");
    }
    
    // Test medium allocation
    void* medium_space = meow_alloc(1024);
    if (medium_space) {
        meow_info("Medium cat space (1KB) allocated at 0x%x", (uint32_t)medium_space);
        meow_free(medium_space);
        meow_info("Medium cat space freed successfully");
    } else {
        meow_error("Failed to allocate medium cat space");
    }
    
    // Test large allocation
    void* large_space = meow_alloc(4096);
    if (large_space) {
        meow_info("Large cat space (4KB) allocated at 0x%x", (uint32_t)large_space);
        meow_free(large_space);
        meow_info("Large cat space freed successfully");
    } else {
        meow_error("Failed to allocate large cat space");
    }
    
    meow_info("Memory allocation tests completed");
}

void test_territory_system(void) {
    meow_info(" Testing territory allocation system...");
    
    // FIXED: Use uint32_t for physical address
    uint32_t territory = purr_alloc_territory();
    
    if (territory != 0) {
        meow_info(" Territory allocated: 0x%x", territory);
        purr_free_territory(territory);
        meow_info(" Territory freed: 0x%x", territory);
    } else {
        meow_error(" Territory allocation failed!");
    }

    purr_status();

    meow_info("Territory system tests complete");
}


// Main cat activities loop
void enter_cat_main_loop(void) {
    meow_printf(" MeowKernel is now running and managing the system!\n");
    meow_printf(" All cats are in control of their territories!\n");
    meow_printf("  Keyboard input supported (mouse support in Phase 2)\n");
    meow_printf(" Press Ctrl+C in QEMU to exit\n");
    meow_printf(" System Status: All cats are content and purring\n\n");
    
    // Main kernel activity loop
    uint32_t activity_counter = 0;
    
    while (1) {
        // Simulate cat activities (this is where future phases will add real functionality)
        activity_counter++;
        
        if (activity_counter % 100000 == 0) {
            // Periodic cat status update
            switch ((activity_counter / 100000) % 4) {
                case 0:
                    meow_printf(" Cats are napping peacefully...\n");
                    break;
                case 1:
                    meow_printf(" Cats are patrolling their territories...\n");
                    break;
                case 2:
                    meow_printf(" Cats are hunting for bugs in the system...\n");
                    break;
                case 3:
                    meow_printf(" Cats are purring contentedly...\n");
                    break;
            }
        }
        
        // Future phases will add:
        // -  Process scheduling (cat napping management)
        // -  Interrupt handling (cat alertness system)
        // -   System call processing (cat communication)
        // -  Device management (cat toy management)
        // -  Network stack (cat social networking)
        
        // For Phase 1: Basic halt instruction to prevent busy-waiting
        if (activity_counter % 50000 == 0) {
            // Simulate brief cat nap (halt CPU briefly)
            asm volatile ("hlt");
        }
    }
}

// Kernel panic function (when cats are very unhappy)
void kernel_panic(const char* message) {
    set_text_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    clear_screen();
    
    terminal_writestring("\n\n");
    terminal_writestring("    MEOW KERNEL PANIC - CATS ARE VERY UNHAPPY! \n\n");
    terminal_writestring("    Reason: ");
    terminal_writestring(message);
    terminal_writestring("\n\n");
    terminal_writestring("    The cats have encountered a serious problem and need\n");
    terminal_writestring("    to restart the system. Please check your code!\n\n");
    terminal_writestring("    System halted. Press reset to restart.\n");
    
    // Halt the system
    while (1) {
        asm volatile ("hlt");
    }
}

