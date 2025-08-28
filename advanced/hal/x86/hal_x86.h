#ifndef HAL_X86_H
#define HAL_X86_H

#include "../hal.h"

/* x86-specific functions */
void x86_gdt_init(void);
void x86_idt_init(void);
void x86_pic_init(void);
void x86_pit_init(uint32_t frequency);

/* Inline assembly helpers */
static inline void x86_cli(void) {
    asm volatile("cli");
}

static inline void x86_sti(void) {
    asm volatile("sti");
}

static inline void x86_hlt(void) {
    asm volatile("hlt");
}

static inline uint32_t x86_get_eflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

#endif /* HAL_X86_H */

