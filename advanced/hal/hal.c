#include "hal.h"

/* Forward declaration for kprintf */
extern void kprintf(const char* format, ...);

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

/* Initialize the HAL */
void hal_init(void) {
    kprintf("HAL: Initializing Hardware Abstraction Layer\n");
    kprintf("HAL: Architecture: %s\n", hal_get_arch_string());
    
    /* Initialize architecture-specific components */
    hal_cpu_init();
    hal_memory_init();
    hal_interrupt_init();
    hal_timer_init(1000); // 1000 Hz timer
    
    kprintf("HAL: Initialization complete\n");
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

