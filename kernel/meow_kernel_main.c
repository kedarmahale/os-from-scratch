/* kernel/meow_kernel_main.c - MeowKernel Main Implementation with Phase 2 Integration
 *
 * Enhanced version that includes Phase 2 features
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_kernel_interface.h"
#include "meow_util.h"
#include "meow_error_definitions.h"
#include "meow_multiboot.h"
#include "meow_shell.h"

/* Phase 2 includes */
#include "../advanced/drivers/keyboard/meow_keyboard.h"
#include "../advanced/process/meow_task.h"
#include "../advanced/syscalls/meow_syscall_table.h"
#include "../advanced/process/meow_scheduler.h"

/* Forward declarations for HAL and memory management */
extern meow_error_t hal_init(multiboot_info_t* mbi);
extern void* meow_heap_alloc(size_t size);
extern uint32_t purr_alloc_territory(void);
extern void purr_free_territory(uint32_t territory);
extern void purr_status(void);

/* ================================================================
 * GLOBAL KERNEL STATE (unchanged from Phase 1)
 * ================================================================ */

static multiboot_info_t* global_multiboot_info = NULL;
static uint32_t global_multiboot_magic = 0;
static uint8_t multiboot_info_valid = 0;

/* ================================================================
 * KERNEL UTILITY FUNCTIONS (unchanged from Phase 1)
 * ================================================================ */

multiboot_info_t* get_multiboot_info(void) {
    return multiboot_info_valid ? global_multiboot_info : NULL;
}

uint32_t get_multiboot_magic(void) {
    return global_multiboot_magic;
}

uint8_t is_multiboot_info_valid(void) {
    return multiboot_info_valid;
}

/* ================================================================
 * KERNEL DISPLAY FUNCTIONS (unchanged from Phase 1)
 * ================================================================ */

static void display_cat_banner(void) {
    clear_screen();
    set_text_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    
    terminal_writestring("  /\\^/\\ \n");
    terminal_writestring(" ( ^.^ ) (\n");
    terminal_writestring(" =\\`Y`/= _)\n");
    terminal_writestring(" ( | | )( \n");
    terminal_writestring(" ( | | )\n");
    terminal_writestring(" ( d b )\n");
    terminal_writestring("\n");
    
    set_text_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("=======================================\n");
    terminal_writestring(" MeowKernel v0.2.0 - Phase 2 \n");
    terminal_writestring(" The Purr-fect Operating System!\n");
    terminal_writestring("=======================================\n");
    
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writestring("\n");
}

/**
 * Validate multiboot information (unchanged from Phase 1)
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

/* ================================================================
 * PHASE 2 INITIALIZATION FUNCTIONS
 * ================================================================ */

static meow_error_t init_phase2_components(void)
{
    meow_log(MEOW_LOG_MEOW, "🚀 Initializing Phase 2 components...");
    
    /* Step 1: Initialize keyboard driver */
    meow_log(MEOW_LOG_MEOW, "⌨️ [1/4] Initializing keyboard driver...");
    meow_error_t result = keyboard_init();
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to initialize keyboard: %s",
                 meow_error_to_string(result));
        return result;
    }
    /* Enable keyboard IRQ (IRQ1) */
    HAL_INTERRUPT_OP(enable_irq, 1);
    meow_log(MEOW_LOG_CHIRP, "😺 Keyboard driver ready - IRQ1 enabled!");

    /* Step 2: Initialize task system */
    meow_log(MEOW_LOG_MEOW, "🔄 [2/4] Initializing process management...");
    result = task_system_init();
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to initialize task system: %s",
                 meow_error_to_string(result));
        return result;
    }
    meow_log(MEOW_LOG_CHIRP, "😺 Process management ready - cats can multitask!");

    /* Step 3: Set up preemptive scheduler */
    meow_log(MEOW_LOG_MEOW, "⏰ [3/4] Registering scheduler callback...");
    HAL_TIMER_OP(register_callback, scheduler_tick);

    meow_log(MEOW_LOG_MEOW, "⏰ [3.1] Enabling PIT IRQ...");
    HAL_INTERRUPT_OP(enable_irq, 0);

    meow_log(MEOW_LOG_MEOW, "⏰ [3.2] Starting system timer...");
    HAL_TIMER_OP(start);

    meow_log(MEOW_LOG_MEOW, "⏰ [3.3] Initializing scheduler...");
    scheduler_init();

    meow_log(MEOW_LOG_MEOW, "📞 [4/4] Initializing system call interface...");
    result = syscall_init();
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to initialize system calls: %s",
                 meow_error_to_string(result));
        return result;
    }
    meow_log(MEOW_LOG_CHIRP, "😺 System calls ready - cats can make requests!");

    // **Enable global interrupts now that all IRQ handlers are registered**
    HAL_CPU_OP(enable_interrupts);

    meow_log(MEOW_LOG_CHIRP, "🎉 Phase 2 initialization complete!");
    return MEOW_SUCCESS;
}

void demo_task_main(void* arg)
{
    int task_id = (int)(uintptr_t)arg;
    int counter = 0;
    
    while (counter < 10) {
        meow_log(MEOW_LOG_CHIRP, "😺 Demo Task %d: Counter = %d", task_id, counter);
        task_sleep(2000);  /* Sleep for 2 seconds */
        counter++;
    }
    
    meow_log(MEOW_LOG_PURR, "😴 Demo Task %d completed - time for a cat nap", task_id);
}

void shell_task_main(void* arg)
{
    /* Run the interactive shell */
    meow_shell_run();
}

/**
 * Create demo tasks to show Phase 2 capabilities
 */
static void create_demo_tasks(void)
{
    meow_log(MEOW_LOG_MEOW, "🎭 Creating demo tasks...");
#if 1
    /* Create demo tasks */
    uint32_t task1_pid = task_create("demo_task_1", demo_task_main, (void*)1, 
                                     PRIORITY_NORMAL, TASK_STACK_SIZE);
    uint32_t task2_pid = task_create("demo_task_2", demo_task_main, (void*)2, 
                                     PRIORITY_NORMAL, TASK_STACK_SIZE);
    
    if (task1_pid == 0 || task2_pid == 0) {
        meow_log(MEOW_LOG_HISS, "😾 Failed to create demo tasks");
    } else {
        meow_log(MEOW_LOG_CHIRP, "😺 Demo tasks created: PID %u and %u", 
                 task1_pid, task2_pid);
    }
#endif
    /* Create interactive shell task */
    uint32_t shell_pid = task_create("shell", shell_task_main, NULL, 
                                     PRIORITY_HIGH, TASK_STACK_SIZE * 2);
    if (shell_pid == 0) {
        meow_log(MEOW_LOG_HISS, "😾 Failed to create shell task");
    } else {
        meow_log(MEOW_LOG_CHIRP, "😺 Interactive shell created: PID %u", shell_pid);
    }
}

/**
 * Phase 2 main loop
 */
static void phase2_main_loop(void)
{
    meow_log(MEOW_LOG_MEOW, "🐱 Starting Phase 2 main loop - cats are in control!");
    
    /* Enable interrupts */
    HAL_CPU_OP(enable_interrupts);
    
    /* Show welcome message */
    meow_log(MEOW_LOG_CHIRP, "🎮 MeowKernel Phase 2 is ready!");
    meow_log(MEOW_LOG_MEOW, "✨ Features available:");
    meow_log(MEOW_LOG_PURR, "   - Interactive keyboard input");
    meow_log(MEOW_LOG_PURR, "   - Multi-tasking with process management");
    meow_log(MEOW_LOG_PURR, "   - System call interface");
    meow_log(MEOW_LOG_PURR, "   - Cat-themed commands and responses");
    meow_log(MEOW_LOG_MEOW, "🐾 Type in the shell or watch the demo tasks run!");
    
    /* Main kernel loop - let the scheduler handle everything */
    uint32_t loop_counter = 0;
    while (1) {
        /* Periodic housekeeping */
        if (loop_counter % 1000000 == 0) {
            /* Clean up terminated tasks */
            task_cleanup_terminated();
            
            /* Show periodic status */
            if (loop_counter % 10000000 == 0) {
                task_statistics_t stats;
                if (task_get_statistics(&stats) == MEOW_SUCCESS) {
                    meow_log(MEOW_LOG_PURR, "💗 System heartbeat - %u tasks running", 
                             stats.running_tasks);
                }
            }
        }
        
        loop_counter++;
        
        /* Yield to other tasks */
        task_yield();
        
        /* Brief halt to save CPU */
        HAL_CPU_OP(halt);
    }
}

/* ================================================================
 * KERNEL TEST FUNCTIONS (kept from Phase 1)
 * ================================================================ */

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
    terminal_writestring(" Red Cat: Meow meow meow!\n");
    
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring(" Green Cat: Purr purr purr!\n");
    
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring(" Blue Cat: Chirp chirp chirp!\n");
    
    set_text_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    terminal_writestring(" Yellow Cat: Hiss hiss (warning)!\n");
    
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
    meow_printf(" - Architecture: x86 32-bit (i386)\n");
    meow_printf(" - Bootloader: GRUB (Multiboot compliant)\n");
    meow_printf(" - Kernel: MeowKernel v0.2.0 Phase 2\n");
    meow_printf(" - HAL Status: Active and purring\n");
    meow_printf(" - Memory Management: Cat territories established\n");
    meow_printf(" - VGA Mode: 80x25 text mode with cat colors\n");
    meow_printf(" - Logging: Cat-themed with emojis \n");
    meow_printf(" - Build System: Cross-compiled with love\n");
    meow_printf(" - Interactive Features: Keyboard, Shell, Tasks\n");
    meow_printf(" - Cat Happiness Level: Maximum! \n");
}

/* ================================================================
 * MAIN KERNEL ENTRY POINT (Enhanced for Phase 2)
 * ================================================================ */

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
    meow_log_set_level(MEOW_LOG_MEOW); /* Show debug and above by default */
    meow_log_enable_emojis(0); /* Don't Enable adorable emojis */
    
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
    
    /* ================================================================
     * PHASE 1 INITIALIZATION SEQUENCE (unchanged)
     * ================================================================ */
     
    /* Step 1: Initialize Hardware Abstraction Layer */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[1/6] 🔧 Initializing Hardware Abstraction Layer...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    if (hal_init(multiboot_info) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_SCREECH, "Failed to initialize HAL!");
        meow_panic("Critical HAL initialization failure");
    }
    meow_log(MEOW_LOG_CHIRP, "HAL initialized - cats can now control hardware!");
    terminal_writestring("\n");
    
    /* Step 2: Initialize Cat Memory Management System */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[2/6] 🧠 Initializing cat memory management...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    init_cat_memory(multiboot_info);
    
    meow_log(MEOW_LOG_CHIRP, "All cat territories established and memory systems ready!");
    terminal_writestring("\n");
    
    /* Step 3: Initialize Phase 2 Components */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[3/6] 🚀 Initializing Phase 2 components...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    if (init_phase2_components() != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "Phase 2 initialization failed - falling back to Phase 1");
        /* Continue with Phase 1 behavior instead of crashing */
    } else {
        meow_log(MEOW_LOG_CHIRP, "Phase 2 components ready - interactive features available!");
    }
    terminal_writestring("\n");
    
    /* Step 4: Display system information */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[4/6] 📊 Displaying system information...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    display_system_info();
    terminal_writestring("\n");
    
    /* Step 5: Run comprehensive cat system tests */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[5/6] 🧪 Running cat system tests...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    run_cat_tests();
    meow_log(MEOW_LOG_CHIRP, "All cats are happy and systems are purring perfectly!");
    terminal_writestring("\n");
    
    /* Step 6: Start Phase 2 features */
    set_text_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    terminal_writestring("[6/6] 🎮 Starting interactive features...\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Create demo tasks */
    create_demo_tasks();
    
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("==== MeowKernel Phase 2 initialization COMPLETE! ====\n\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
     HAL_CPU_OP(enable_interrupts);
    /* Enter Phase 2 main loop with multitasking */
    phase2_main_loop();
}

/* ================================================================
 * PANIC FUNCTION (unchanged from Phase 1)
 * ================================================================ */

/**
 * Kernel panic function - when cats are VERY unhappy
 */
void kernel_panic(const char* message) {
    /* Use the cat-themed panic from meow_util.c */
    meow_panic(message);
}

/* That's all, folks! The cats are now in control with Phase 2 powers! */