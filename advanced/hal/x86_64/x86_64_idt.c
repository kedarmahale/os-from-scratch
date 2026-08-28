#include <stdint.h>
#include "x86_64_interrupts.h"

#ifdef MEOW_TEST_MODE
#ifndef X86_64_TEST_EXCEPTION_VECTOR
#define X86_64_TEST_EXCEPTION_VECTOR 255
#endif
extern void x86_64_test_exception_pass(uint64_t vector);
extern void x86_64_test_exception_fail(uint64_t vector);
extern void x86_64_test_breakpoint_handled(uint64_t vector);
#endif

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} idt_entry64_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idtr64_t;

static idt_entry64_t idt[256];
static idtr64_t idtr;

static const char *const exception_names[32] = {
    "0 (divide error)",
    "1 (debug)",
    "2 (non-maskable interrupt)",
    "3 (breakpoint)",
    "4 (overflow)",
    "5 (bound range exceeded)",
    "6 (invalid opcode)",
    "7 (device not available)",
    "8 (double fault)",
    "9 (coprocessor segment overrun)",
    "10 (invalid TSS)",
    "11 (segment not present)",
    "12 (stack fault)",
    "13 (general protection fault)",
    "14 (page fault)",
    "15 (reserved)",
    "16 (x87 floating-point exception)",
    "17 (alignment check)",
    "18 (machine check)",
    "19 (SIMD floating-point exception)",
    "20 (virtualization exception)",
    "21 (control protection exception)",
    "22 (reserved)",
    "23 (reserved)",
    "24 (reserved)",
    "25 (reserved)",
    "26 (reserved)",
    "27 (reserved)",
    "28 (hypervisor injection exception)",
    "29 (VMM communication exception)",
    "30 (security exception)",
    "31 (reserved)"
};

static void exception_write(const char *text)
{
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    static uint32_t cursor = 0;
    while (*text && cursor < 80 * 25) {
        char c = *text++;
        if (c == '\n') cursor = (cursor / 80 + 1) * 80;
        else vga[cursor++] = (uint16_t)c | 0x4F00;
    }
}

extern void x86_64_load_idt(const idtr64_t *descriptor);
extern void x86_64_exception_stub0(void);
extern void x86_64_exception_stub1(void);
extern void x86_64_exception_stub2(void);
extern void x86_64_exception_stub3(void);
extern void x86_64_exception_stub4(void);
extern void x86_64_exception_stub5(void);
extern void x86_64_exception_stub6(void);
extern void x86_64_exception_stub7(void);
extern void x86_64_exception_stub8(void);
extern void x86_64_exception_stub9(void);
extern void x86_64_exception_stub10(void);
extern void x86_64_exception_stub11(void);
extern void x86_64_exception_stub12(void);
extern void x86_64_exception_stub13(void);
extern void x86_64_exception_stub14(void);
extern void x86_64_exception_stub15(void);
extern void x86_64_exception_stub16(void);
extern void x86_64_exception_stub17(void);
extern void x86_64_exception_stub18(void);
extern void x86_64_exception_stub19(void);
extern void x86_64_exception_stub20(void);
extern void x86_64_exception_stub21(void);
extern void x86_64_exception_stub22(void);
extern void x86_64_exception_stub23(void);
extern void x86_64_exception_stub24(void);
extern void x86_64_exception_stub25(void);
extern void x86_64_exception_stub26(void);
extern void x86_64_exception_stub27(void);
extern void x86_64_exception_stub28(void);
extern void x86_64_exception_stub29(void);
extern void x86_64_exception_stub30(void);
extern void x86_64_exception_stub31(void);
extern void x86_64_unexpected_stub(void);
extern void x86_64_irq_stub0(void);
extern void x86_64_irq_stub1(void);
extern void x86_64_irq_stub2(void);
extern void x86_64_irq_stub3(void);
extern void x86_64_irq_stub4(void);
extern void x86_64_irq_stub5(void);
extern void x86_64_irq_stub6(void);
extern void x86_64_irq_stub7(void);
extern void x86_64_irq_stub8(void);
extern void x86_64_irq_stub9(void);
extern void x86_64_irq_stub10(void);
extern void x86_64_irq_stub11(void);
extern void x86_64_irq_stub12(void);
extern void x86_64_irq_stub13(void);
extern void x86_64_irq_stub14(void);
extern void x86_64_irq_stub15(void);

typedef void (*exception_stub_t)(void);

static const exception_stub_t exception_stubs[32] = {
    x86_64_exception_stub0,  x86_64_exception_stub1,
    x86_64_exception_stub2,  x86_64_exception_stub3,
    x86_64_exception_stub4,  x86_64_exception_stub5,
    x86_64_exception_stub6,  x86_64_exception_stub7,
    x86_64_exception_stub8,  x86_64_exception_stub9,
    x86_64_exception_stub10, x86_64_exception_stub11,
    x86_64_exception_stub12, x86_64_exception_stub13,
    x86_64_exception_stub14, x86_64_exception_stub15,
    x86_64_exception_stub16, x86_64_exception_stub17,
    x86_64_exception_stub18, x86_64_exception_stub19,
    x86_64_exception_stub20, x86_64_exception_stub21,
    x86_64_exception_stub22, x86_64_exception_stub23,
    x86_64_exception_stub24, x86_64_exception_stub25,
    x86_64_exception_stub26, x86_64_exception_stub27,
    x86_64_exception_stub28, x86_64_exception_stub29,
    x86_64_exception_stub30, x86_64_exception_stub31
};

static const exception_stub_t irq_stubs[16] = {
    x86_64_irq_stub0,  x86_64_irq_stub1,
    x86_64_irq_stub2,  x86_64_irq_stub3,
    x86_64_irq_stub4,  x86_64_irq_stub5,
    x86_64_irq_stub6,  x86_64_irq_stub7,
    x86_64_irq_stub8,  x86_64_irq_stub9,
    x86_64_irq_stub10, x86_64_irq_stub11,
    x86_64_irq_stub12, x86_64_irq_stub13,
    x86_64_irq_stub14, x86_64_irq_stub15
};

static void idt_set_gate(uint8_t vector, uintptr_t handler)
{
    idt_entry64_t *gate = &idt[vector];
    gate->offset_low = (uint16_t)(handler & 0xffffU);
    gate->selector = 0x08;       /* kernel 64-bit code segment */
    gate->ist = 0;
    gate->type_attr = 0x8e;      /* present, ring 0, interrupt gate */
    gate->offset_mid = (uint16_t)((handler >> 16) & 0xffffU);
    gate->offset_high = (uint32_t)(handler >> 32);
    gate->reserved = 0;
}

void x86_64_idt_init(void)
{
    for (uint32_t vector = 0; vector < 256; ++vector) {
        idt_set_gate((uint8_t)vector, (uintptr_t)x86_64_unexpected_stub);
    }
    for (uint32_t vector = 0; vector < 32; ++vector) {
        idt_set_gate((uint8_t)vector, (uintptr_t)exception_stubs[vector]);
    }
    for (uint32_t irq = 0; irq < 16; ++irq) {
        idt_set_gate((uint8_t)(32 + irq), (uintptr_t)irq_stubs[irq]);
    }
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base = (uint64_t)(uintptr_t)idt;
    x86_64_load_idt(&idtr);
}

void x86_64_exception_handler(const x86_64_interrupt_frame_t *frame)
{
    exception_write("EXCEPTION: CPU fault, system halted\n");
    if (frame && frame->vector < 32) {
        exception_write("Vector: ");
        exception_write(exception_names[frame->vector]);
        exception_write("\n");
    } else {
        exception_write("Vector: unhandled\n");
    }

#ifdef MEOW_TEST_MODE
    if (frame && frame->vector == X86_64_TEST_EXCEPTION_VECTOR) {
        /* #BP is deliberately returned from to validate the iretq path. */
        if (frame->vector == 3) {
            x86_64_test_breakpoint_handled(frame->vector);
            return;
        }
        x86_64_test_exception_pass(frame->vector);
    }
    x86_64_test_exception_fail(frame ? frame->vector : 255);
#endif

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
