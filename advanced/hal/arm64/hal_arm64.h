/* advanced/hal/arm64/hal_arm64.h - ARM64 HAL interface */

#ifndef HAL_ARM64_H
#define HAL_ARM64_H

#include "../hal.h"

/* ARM64-specific functions */
void arm64_mmu_init(void);
void arm64_gic_init(void);
void arm64_timer_init(uint32_t frequency);
void arm64_uart_init(void);

/* ARM64 inline helpers */
static inline void arm64_disable_interrupts(void) {
    asm volatile("msr daifset, #2" ::: "memory");
}

static inline void arm64_enable_interrupts(void) {
    asm volatile("msr daifclr, #2" ::: "memory");
}

static inline void arm64_halt(void) {
    asm volatile("wfi" ::: "memory");
}

static inline uint64_t arm64_get_current_el(void) {
    uint64_t el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    return (el >> 2) & 3;
}

#endif /* HAL_ARM64_H */
