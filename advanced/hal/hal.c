#include "hal.h"
#include "../mm/territory_map.h"
#include "../../kernel/meow_kernel.h"
#include "../../kernel/meow_util.h"

/* Include architecture-specific implementations */
#ifdef __i386__
    #include "x86/hal_x86.h"
    #define CURRENT_ARCH ARCH_X86
    #define ARCH_STRING "i386"
#elif __x86_64__
    #include "x86_64/hal_x86_64.h"
    #define CURRENT_ARCH ARCH_X86_64
    #define ARCH_STRING "x86_64"
#elif __arm__
    #include "arm/hal_arm.h"
    #define CURRENT_ARCH ARCH_ARM
    #define ARCH_STRING "arm"
#elif __aarch64__
    #include "arm/hal_aarch64.h"
    #define CURRENT_ARCH ARCH_AARCH64
    #define ARCH_STRING "aarch64"
#else
    #define CURRENT_ARCH ARCH_UNKNOWN
    #define ARCH_STRING "unknown"
#endif

/* Global HAL state */
static hal_arch_t current_architecture = CURRENT_ARCH;
static uint64_t system_ticks = 0;
static uint8_t hal_initialized = 0;

void hal_init(void) {
    meow_info("==== Initializing Hardware Abstraction Layer... ====");
    
    /* Get multiboot info from kernel if available */
    multiboot_info_t* mbi = get_multiboot_info();
    
    if (mbi && is_multiboot_info_valid()) {
        meow_info(" HAL: Using multiboot ino for hardware detection");
        
        // Pass multiboot info to x86 HAL for memory detection
        #ifdef TARGET_X86
            hal_set_multiboot_info(mbi);
        #endif
    } else {
        meow_warn(" HAL: No valid multiboot info - using defaults!!!");
    }

    /* Initialize CPU and basic systems */
    hal_cpu_init();
    
    /* Initialize memory detection */
    hal_memory_init();
    
    /* Initialize timer */
    hal_timer_init(1000); /* 1000 Hz timer */
    
    /* Initialize interrupts */
    hal_interrupt_init();
    
    /* Mark HAL as initialized */
    hal_set_initialized();
    
    /* Show system information */
    hal_show_system_info();
    
    /* Run self-test */
    if (!hal_self_test()) {
        hal_emergency_halt("HAL self-test failed!!!");
    }
    
    meow_info("==== Hardware Abstraction Layer initialized successfully ====");
}

/* Architecture detection */
hal_arch_t hal_get_architecture(void) {
    return current_architecture;
}

const char* hal_get_arch_string(void) {
    return ARCH_STRING;
}

/* Timer tick increment (called by timer interrupt) */
void hal_timer_tick(void) {
    system_ticks++;
}

/* Get system ticks */
uint64_t hal_timer_get_ticks(void) {
    return system_ticks;
}

