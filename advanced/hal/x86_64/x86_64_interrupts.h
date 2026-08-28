#ifndef MEOWKERNEL_X86_64_INTERRUPTS_H
#define MEOWKERNEL_X86_64_INTERRUPTS_H

#include <stdint.h>
#include <stddef.h>

/*
 * The assembly entry path builds this exact ring-0 frame before calling C.
 * The first field is at the address passed in RDI.  The processor-provided
 * RIP/CS/RFLAGS follow the synthetic vector and error-code fields.
 */
typedef struct __attribute__((packed)) x86_64_interrupt_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} x86_64_interrupt_frame_t;

/* Keep the C view tied to the push/pop order in boot/x86_64/interrupts.S. */
_Static_assert(offsetof(x86_64_interrupt_frame_t, r15) == 0, "bad interrupt frame");
_Static_assert(offsetof(x86_64_interrupt_frame_t, rax) == 14 * sizeof(uint64_t), "bad register frame");
_Static_assert(offsetof(x86_64_interrupt_frame_t, vector) == 15 * sizeof(uint64_t), "bad vector frame");
_Static_assert(offsetof(x86_64_interrupt_frame_t, error_code) == 16 * sizeof(uint64_t), "bad error frame");
_Static_assert(offsetof(x86_64_interrupt_frame_t, rip) == 17 * sizeof(uint64_t), "bad return frame");
_Static_assert(sizeof(x86_64_interrupt_frame_t) == 20 * sizeof(uint64_t), "bad interrupt frame size");

typedef void (*x86_64_irq_handler_t)(x86_64_interrupt_frame_t *frame);
typedef x86_64_interrupt_frame_t *(*x86_64_irq_switch_handler_t)(x86_64_interrupt_frame_t *frame);

x86_64_interrupt_frame_t *x86_64_irq_dispatch(x86_64_interrupt_frame_t *frame);
void x86_64_pic_init(void);
void x86_64_pic_enable_irq(uint8_t irq);
void x86_64_pic_disable_irq(uint8_t irq);
void x86_64_pic_eoi(uint8_t irq);
void x86_64_pit_init(uint32_t frequency);
void x86_64_irq_register(uint8_t irq, x86_64_irq_handler_t handler);
void x86_64_irq_register_timer_switch(x86_64_irq_switch_handler_t handler);
uint64_t x86_64_timer_ticks(void);

static inline void x86_64_cli(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

static inline void x86_64_sti(void)
{
    __asm__ volatile ("sti" ::: "memory");
}

static inline void x86_64_hlt(void)
{
    __asm__ volatile ("hlt");
}

static inline uint8_t x86_64_inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void x86_64_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void x86_64_outl(uint16_t port, uint32_t value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline void x86_64_io_wait(void)
{
    x86_64_outb(0x80, 0);
}

#endif
