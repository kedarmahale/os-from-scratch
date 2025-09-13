/* advanced/hal/x86/hal_x86.h - Enhanced x86 HAL interface */

#ifndef HAL_X86_H
#define HAL_X86_H

#include "../hal.h"

/* x86-specific initialization functions */
void x86_gdt_init(void);
void x86_idt_init(void);
void x86_pic_init(void);
void x86_pit_init(uint32_t frequency);

/* PIC functions */
void x86_pic_eoi(uint8_t irq);
void x86_pic_enable_irq(uint8_t irq);
void x86_pic_disable_irq(uint8_t irq);

/* Timer functions */
uint32_t x86_pit_get_frequency(void);
void x86_pit_sleep(uint32_t milliseconds);

/* Interrupt handling */
void interrupt_handler(uint32_t interrupt_number, uint32_t error_code);

/* Assembly helper functions */
extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void idt_flush(uint32_t idt_ptr_addr);

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

/* Interrupt acknowledgment */
static inline void x86_irq_ack(uint8_t irq) {
    x86_pic_eoi(irq);
}

#endif /* HAL_X86_H */
