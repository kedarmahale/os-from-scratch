/* advanced/hal/x86/idt.c - Interrupt Descriptor Table Implementation */

#include "hal_x86.h"

/* Forward declaration for meow_kprintf */
extern void meow_printf(const char* format, ...);

/* IDT Entry Structure */
struct idt_entry {
    uint16_t base_low;       // Lower 16 bits of handler address
    uint16_t selector;       // Kernel segment selector
    uint8_t  always0;        // Always 0
    uint8_t  flags;          // Flags and gate type
    uint16_t base_high;      // Upper 16 bits of handler address
} __attribute__((packed));

/* IDT Pointer Structure */
struct idt_ptr {
    uint16_t limit;          // Size of IDT
    uint32_t base;           // Base address of IDT
} __attribute__((packed));

/* IDT Entries */
static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

/* External interrupt handler stubs from interrupts.S */
extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void);
extern void isr5(void), isr6(void), isr7(void), isr8(void), isr9(void);
extern void isr10(void), isr11(void), isr12(void), isr13(void), isr14(void);
extern void isr15(void), isr16(void), isr17(void), isr18(void), isr19(void);
extern void isr20(void), isr21(void), isr22(void), isr23(void), isr24(void);
extern void isr25(void), isr26(void), isr27(void), isr28(void), isr29(void);
extern void isr30(void), isr31(void);

extern void irq0(void), irq1(void), irq2(void), irq3(void), irq4(void);
extern void irq5(void), irq6(void), irq7(void), irq8(void), irq9(void);
extern void irq10(void), irq11(void), irq12(void), irq13(void), irq14(void);
extern void irq15(void);

/* Set an IDT entry */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/* Initialize the IDT */
void x86_idt_init(void) {
    meow_printf("x86: Initializing Interrupt Descriptor Table\n");
    
    /* Setup IDT pointer */
    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;

    /* Clear IDT */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    /* Setup exception handlers (ISR 0-31) */
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);    // Division by zero
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);    // Debug
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);    // NMI
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);    // Breakpoint
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);    // Overflow
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);    // Bound range exceeded
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);    // Invalid opcode
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);    // Device not available
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);    // Double fault
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);    // Coprocessor segment overrun
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);  // Invalid TSS
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);  // Segment not present
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);  // Stack fault
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);  // General protection fault
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);  // Page fault
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);  // Reserved
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);  // x87 FPU error
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);  // Alignment check
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);  // Machine check
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);  // SIMD FP exception

    /* Setup IRQ handlers (IRQ 0-15 -> INT 32-47) */
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);   // Timer
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);   // Keyboard
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);   // Cascade
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);   // Serial port 2
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);   // Serial port 1
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);   // Parallel port 2
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);   // Floppy disk
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);   // Parallel port 1
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);   // Real-time clock
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);   // ACPI
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);  // Available
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);  // Available
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);  // PS/2 mouse
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);  // FPU
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);  // Primary ATA
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);  // Secondary ATA

    /* Load the IDT */
    idt_flush((uint32_t)&idt_ptr);
    
    meow_printf("x86: IDT initialized with %d interrupt vectors\n", 256);
}

/* Exception names for debugging */
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception"
};

/* Common interrupt handler called from assembly stubs */
void interrupt_handler(uint32_t interrupt_number, uint32_t error_code) {
    if (interrupt_number < 32) {
        /* CPU Exception */
        const char* exception_name = (interrupt_number < 20) ? 
            exception_messages[interrupt_number] : "Reserved Exception";
        
        meow_printf("\n EXCEPTION: %s (INT %u, Error: 0x%08X)\n", 
                exception_name, interrupt_number, error_code);
        
        /* For now, halt on exceptions */
        meow_printf("System halted due to exception.\n");
        while (1) {
            x86_hlt();
        }
    } 
    else if (interrupt_number >= 32 && interrupt_number <= 47) {
        /* Hardware IRQ */
        uint8_t irq = interrupt_number - 32;
        
        switch (irq) {
            case 0: /* Timer IRQ */
                hal_timer_tick();
                break;
                
            case 1: /* Keyboard IRQ */
                meow_printf("⌨️Keyboard interrupt\n");
                break;
                
            default:
                meow_printf(" IRQ %u received\n", irq);
                break;
        }
        
        /* Send End-of-Interrupt to PIC */
        x86_pic_eoi(irq);
    }
    else {
        /* Unknown interrupt */
        meow_printf(" Unknown interrupt: %u\n", interrupt_number);
    }
}
