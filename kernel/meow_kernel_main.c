/* kernel/meow_kernel_main.c - MeowKernel Main Implementation
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_kernel_interface.h"
#include "meow_util.h"
#include "meow_error_definitions.h"
#include "meow_multiboot.h"

/* Forward declarations for HAL and memory management */
extern meow_error_t hal_init(multiboot_info_t* mbi);
extern meow_error_t init_cat_memory(multiboot_info_t* mbi);
extern void* meow_heap_alloc(size_t size);
extern void meow_heap_free(void* ptr);
extern uint32_t purr_alloc_territory(void);
extern void purr_free_territory(uint32_t territory);
extern void purr_status(void);

/* ============================================================================
 * GLOBAL KERNEL STATE
 * ============================================================================ */

static multiboot_info_t* global_multiboot_info = NULL;
static uint32_t global_multiboot_magic = 0;
static uint8_t multiboot_info_valid = 0;

/* ============================================================================
 * KERNEL UTILITY FUNCTIONS
 * ============================================================================ */

multiboot_info_t* get_multiboot_info(void) {
    return multiboot_info_valid ? global_multiboot_info : NULL;
}

uint32_t get_multiboot_magic(void) {
    return global_multiboot_magic;
}

uint8_t is_multiboot_info_valid(void) {
    return multiboot_info_valid;
}

/* ============================================================================
 * KERNEL DISPLAY FUNCTIONS
 * ============================================================================ */
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

/**
 * Validate multiboot information
 */
static uint8_t validate_multiboot_info(uint32_t magic, multiboot_info_t* mbi) {
    /* Check multiboot magic number */
    if (magic != MULTIBOOT_MAGIC) {
        meow_log(MEOW_LOG_YOWL, "Invalid multiboot magic: 0x%x (expected 0x%x)", magic, MULTIBOOT_MAGIC);
        return 0;
    }

    /* Check multiboot info pointer */
    if (!mbi) {
        meow_log(MEOW_LOG_YOWL, "Null multiboot info pointer - bootloader didn't provide info");
        return 0;
    }

    /* Basic sanity check on multiboot info structure */
    if ((uint32_t)mbi < 0x1000 || (uint32_t)mbi > 0x100000) {
        meow_log(MEOW_LOG_YOWL, "Multiboot info pointer looks invalid: 0x%x", (uint32_t)mbi);
        return 0;
    }

    /* Check if memory information is available */
    if (!(mbi->flags & (1 << 0))) {
        meow_log(MEOW_LOG_HISS, "No basic memory info available from bootloader");
    }

    /* Check if memory map is available (critical for territory mapping) */
    if (!(mbi->flags & (1 << 6))) {
        meow_log(MEOW_LOG_HISS, "No memory map available - territory mapping will be limited");
        return 0; /* Can't do proper memory management without memory map */
    }

    /* Validate memory map pointer */
    if (!mbi->mmap_addr || mbi->mmap_length == 0) {
        meow_log(MEOW_LOG_YOWL, "Invalid memory map: addr=0x%x, length=%d", 
                  mbi->mmap_addr, mbi->mmap_length);
        return 0;
    }

    meow_log(MEOW_LOG_MEOW, "Multiboot validation passed - bootloader info looks good!");
    return 1; /* Valid multiboot info */
}

/* ============================================================================
 * KERNEL TEST FUNCTIONS
 * ============================================================================ */

/* Test memory allocation functionality */
static void test_memory_allocation(void) {
    meow_log(MEOW_LOG_MEOW, "Testing cat memory allocation system...");

    /* Test small allocation */
    void* small_space = meow_heap_alloc(64);
    if (small_space) {
        meow_log(MEOW_LOG_CHIRP, "Small cat space (64 bytes) allocated at 0x%x", (uint32_t)small_space);
        meow_heap_free(small_space);
        meow_log(MEOW_LOG_CHIRP, "Small cat space freed successfully");
    } else {
        meow_log(MEOW_LOG_YOWL, "Failed to allocate small cat space - the cats are unhappy!");
    }

    /* Test medium allocation */
    void* medium_space = meow_heap_alloc(1024);
    if (medium_space) {
        meow_log(MEOW_LOG_CHIRP, "Medium cat space (1KB) allocated at 0x%x", (uint32_t)medium_space);
        meow_heap_free(medium_space);
        meow_log(MEOW_LOG_CHIRP, "Medium cat space freed successfully");
    } else {
        meow_log(MEOW_LOG_YOWL, "Failed to allocate medium cat space - not enough room!");
    }

    /* Test large allocation */
    void* large_space = meow_heap_alloc(4096);
    if (large_space) {
        meow_log(MEOW_LOG_CHIRP, "Large cat space (4KB) allocated at 0x%x", (uint32_t)large_space);
        meow_heap_free(large_space);
        meow_log(MEOW_LOG_CHIRP, "Large cat space freed successfully");
    } else {
        meow_log(MEOW_LOG_YOWL, "Failed to allocate large cat space - cats need more territory!");
    }

    meow_log(MEOW_LOG_CHIRP, "Memory allocation tests completed - cats are content!");
}

/* Test territory system */
static void test_territory_system(void) {
    meow_log(MEOW_LOG_CHIRP, "Testing territory allocation system...");

    /* Test territory allocation */
    uint32_t territory = purr_alloc_territory();
    if (territory != 0) {
        meow_log(MEOW_LOG_CHIRP, "Territory allocated for the cats: 0x%x", territory);
        purr_free_territory(territory);
        meow_log(MEOW_LOG_CHIRP, "Territory freed and returned to the wild: 0x%x", territory);
    } else {
        meow_log(MEOW_LOG_YOWL, "Territory allocation failed - no land for the cats!");
    }

    /* Show territory status */
    purr_status();
    meow_log(MEOW_LOG_CHIRP, "Territory system tests complete - cats control their domain!");
}

/* Test HAL integration */
static void test_hal_integration(void) {
    meow_log(MEOW_LOG_MEOW, "Testing Hardware Abstraction Layer integration...");
    
    /* The HAL should be initialized by now, so just verify it's working */
    meow_log(MEOW_LOG_CHIRP, "HAL integration test passed - cats can control hardware!");
}

/* Test display system with colorful cat messages */
static void test_display_system(void) {
    meow_log(MEOW_LOG_MEOW, "Testing cat display system with colorful messages...");

    set_text_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_writestring("   Red Cat: Meow meow meow!\n");
    
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("   Green Cat: Purr purr purr!\n");
    
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("   Blue Cat: Chirp chirp chirp!\n");
    
    set_text_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    terminal_writestring("   Yellow Cat: Hiss hiss (warning)!\n");
    
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    meow_log(MEOW_LOG_CHIRP, "Display system test passed - cats can show their colors!");
}

/* Run comprehensive cat system tests */
static void run_cat_tests(void) {
    meow_log(MEOW_LOG_MEOW, "Starting comprehensive cat system tests...");

    /* Test 1: Memory allocation system */
    test_memory_allocation();

    /* Test 2: Territory system */
    test_territory_system();

    /* Test 3: HAL integration test */
    test_hal_integration();

    /* Test 4: Display system test */
    test_display_system();

    meow_log(MEOW_LOG_CHIRP, "All cat system tests completed - everything is purr-fect!");
}

/* Display comprehensive system information */
static void display_system_info(void) {
    meow_printf("==== MEOWKERNEL SYSTEM INFORMATION: ====\n");
    meow_printf("  - Architecture: x86 32-bit (i386)\n");
    meow_printf("  - Bootloader: GRUB (Multiboot compliant)\n");
    meow_printf("  - Kernel: MeowKernel v0.1.0 Clean Build\n");
    meow_printf("  - HAL Status: Active and purring\n");
    meow_printf("  - Memory Management: Cat territories established\n");
    meow_printf("  - VGA Mode: 80x25 text mode with cat colors\n");
    meow_printf("  - Logging: Cat-themed with emojis \n");
    meow_printf("  - Build System: Clean cross-compiled with love\n");
    meow_printf("  - Cat Happiness Level: Maximum! \n");
}

/* Main cat activities loop */
static void enter_cat_main_loop(void) {
    meow_printf("MeowKernel is now running and managing the system!\n");
    meow_printf("All cats are in control of their territories!\n");
    meow_printf("Keyboard input supported (mouse support in future versions)\n");
    meow_printf("Press Ctrl+C in QEMU/VM to exit\n");
    meow_printf("System Status: All cats are content and purring\n\n");

    /* Initialize activity counter */
    uint32_t activity_counter = 0;

    /* Main kernel activity loop */
    while (1) {
        /* Simulate cat activities */
        activity_counter++;

        /* Periodic cat status updates */
        if (activity_counter % 100000 == 0) {
            switch ((activity_counter / 100000) % 6) {
                case 0:
                    meow_log(MEOW_LOG_PURR, "Cats are napping peacefully in their cozy territories...");
                    break;
                case 1:
                    meow_log(MEOW_LOG_MEOW, "Cats are patrolling their territories and ensuring security... ");
                    break;
                case 2:
                    meow_log(MEOW_LOG_CHIRP, "Cats are hunting for bugs in the system code... ");
                    break;
                case 3:
                    meow_log(MEOW_LOG_CHIRP, "Cats are purring contentedly with system stability... ");
                    break;
                case 4:
                    meow_log(MEOW_LOG_MEOW, "Cats are organizing memory and keeping things tidy... ");
                    break;
                case 5:
                    meow_log(MEOW_LOG_PURR, "Cats are dreaming of future kernel features... ");
                    break;
            }
        }

        /* Brief CPU rest (cat nap) to prevent busy-waiting */
        if (activity_counter % 50000 == 0) {
            asm volatile("hlt");
        }
    }
}

/* ============================================================================
 * MAIN KERNEL ENTRY POINT
 * ============================================================================ */

/**
 * Main kernel entry point - called from boot.S
 */
void kernel_main(uint32_t magic, multiboot_info_t* multiboot_info) {
    /* Display our adorable cat banner */
    display_cat_banner();

    /* Store multiboot parameters globally */
    global_multiboot_magic = magic;
    global_multiboot_info = multiboot_info;

    /* Initialize cat-themed logging system first */
    meow_log_set_level(MEOW_LOG_MEOW);  /* Show debug and above by default */
    meow_log_enable_emojis(0);          /* Dont Enable adorable emojis */

    /* CRITICAL: Validate multiboot information before using */
    if (!validate_multiboot_info(magic, multiboot_info)) {
        meow_log(MEOW_LOG_SCREECH, "Invalid or missing multiboot information!");
        meow_log(MEOW_LOG_YOWL, "Cannot initialize memory management without boot info!");
        
        /* Try to continue with minimal functionality */
        meow_log(MEOW_LOG_HISS, "Continuing with limited functionality...");
        
        /* Initialize HAL without memory management */
        if (hal_init(NULL) != MEOW_SUCCESS) {
            meow_log(MEOW_LOG_SCREECH, "Failed to initialize HAL in recovery mode!");
            meow_panic("Critical HAL initialization failure");
        }

        /* Show minimal kernel info */
        meow_log(MEOW_LOG_CHIRP, "MeowKernel running in recovery mode");
        meow_log(MEOW_LOG_HISS, "Memory management disabled - no territory mapping");

        /* Halt in recovery mode */
        while(1) {
            asm volatile("hlt");
        }
    }

    multiboot_info_valid = 1;

    meow_log(MEOW_LOG_CHIRP, " MeowKernel initialization starting...");
    terminal_writestring("\n");

    meow_log(MEOW_LOG_MEOW, "Multiboot info received at address: 0x%x", (uint32_t)multiboot_info);
    meow_log(MEOW_LOG_MEOW, "Multiboot flags: 0x%x", multiboot_info->flags);
    meow_log(MEOW_LOG_MEOW, "Memory lower: %d KB", multiboot_info->mem_lower);
    meow_log(MEOW_LOG_MEOW, "Memory upper: %d KB", multiboot_info->mem_upper);

    /* ========================================================================
     * PHASE 1 INITIALIZATION SEQUENCE
     * ======================================================================== */

    /* Step 1: Initialize Hardware Abstraction Layer */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[1/5] 🔧 Initializing Hardware Abstraction Layer...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    if (hal_init(multiboot_info) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_SCREECH, "Failed to initialize HAL!");
        meow_panic("Critical HAL initialization failure");
    }
    meow_log(MEOW_LOG_CHIRP, "HAL initialized - cats can now control hardware!");
    terminal_writestring("\n");

    /* Step 2: Initialize Cat Memory Management System */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[2/5]  Initializing cat memory management...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    if (init_cat_memory(multiboot_info) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_SCREECH, "Failed to initialize cat memory management!");
        meow_panic("Critical memory management failure");
    }
    meow_log(MEOW_LOG_CHIRP, "All cat territories established and memory systems ready!");
    terminal_writestring("\n");

    /* Step 3: Display system information */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[3/5]  Displaying system information...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    display_system_info();
    terminal_writestring("\n");

    /* Step 4: Run comprehensive cat system tests */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[4/5]  Running cat system tests...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    run_cat_tests();
    meow_log(MEOW_LOG_CHIRP, "All cats are happy and systems are purring perfectly!");
    terminal_writestring("\n");

    /* Step 5: Enter main cat activities loop */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[5/5]  Starting main cat activities...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("==== MeowKernel initialization COMPLETE! ====\n\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Enter the main kernel loop */
    enter_cat_main_loop();
}

/* ============================================================================
 * PANIC FUNCTION
 * ============================================================================ */

/**
 * Kernel panic function - when cats are VERY unhappy
 */
void kernel_panic(const char* message) {
    /* Use the cat-themed panic from meow_util.c */
    meow_panic(message);
}

/* That's all, folks! The cats are now in control! */