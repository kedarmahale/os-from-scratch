/* advanced/hal/x86/hal_x86.c - Enhanced x86 HAL implementation */

#include "hal_x86.h"

/* Forward declaration for meow_printf */
extern void meow_printf(const char* format, ...);

/* Port I/O Implementation - AT&T syntax inline assembly */
uint8_t hal_port_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %w1, %b0" : "=a"(result) : "Nd"(port));
    return result;
}

uint16_t hal_port_inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %w1, %w0" : "=a"(result) : "Nd"(port));
    return result;
}

uint32_t hal_port_inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %w1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void hal_port_outb(uint16_t port, uint8_t data) {
    asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

void hal_port_outw(uint16_t port, uint16_t data) {
    asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
}

void hal_port_outl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %w1" : : "a"(data), "Nd"(port));
}

/* CPU Operations */
void hal_cpu_init(void) {
    meow_printf("HAL: Initializing x86 CPU\n");
    
    /* Initialize GDT first (memory segmentation) */
    x86_gdt_init();
    
    /* Initialize IDT (interrupt handling) */
    x86_idt_init();
    
    /* Initialize PIC (interrupt controller) */
    x86_pic_init();
    
    meow_printf("HAL: x86 CPU initialization complete\n");
}

void hal_cpu_halt(void) {
    x86_hlt();
}

void hal_cpu_disable_interrupts(void) {
    x86_cli();
}

void hal_cpu_enable_interrupts(void) {
    x86_sti();
}

uint32_t hal_cpu_get_flags(void) {
    return x86_get_eflags();
}

/* Memory Operations */
void hal_memory_init(void) {
    meow_printf("HAL: Initializing x86 memory management\n");
    
    /* For now, we'll use simple fixed values */
    /* TODO: Implement proper memory detection using multiboot info */
    
    meow_printf("HAL: x86 memory management initialized\n");
}

uint32_t hal_memory_get_total_size(void) {
    /* TODO: Get actual memory size from multiboot info */
    return 128 * 1024 * 1024; /* 128MB default */
}

uint32_t hal_memory_get_available_size(void) {
    /* Reserve space for kernel and essential structures */
    uint32_t total = hal_memory_get_total_size();
    uint32_t reserved = 8 * 1024 * 1024; /* Reserve 8MB for kernel */
    
    return (total > reserved) ? (total - reserved) : 0;
}

void* hal_memory_get_kernel_end(void) {
    extern char _kernel_end;
    return &_kernel_end;
}

/* Timer Operations */
void hal_timer_init(uint32_t frequency) {
    meow_printf("HAL: Initializing x86 timer at %u Hz\n", frequency);
    
    /* Initialize PIT (Programmable Interval Timer) */
    x86_pit_init(frequency);
    
    meow_printf("HAL: x86 timer initialization complete\n");
}

void hal_timer_sleep(uint32_t milliseconds) {
    x86_pit_sleep(milliseconds);
}

/* Interrupt Operations */
void hal_interrupt_init(void) {
    meow_printf("HAL: Initializing x86 interrupt system\n");
    
    /* IDT and PIC are initialized in hal_cpu_init() */
    
    meow_printf("HAL: x86 interrupt system initialized\n");
}

void hal_interrupt_register_handler(uint8_t interrupt, void (*handler)(void)) {
    /* Suppress unused parameter warnings */
    (void)interrupt;
    (void)handler;
    
    /* TODO: Implement dynamic interrupt handler registration */
    meow_printf("HAL: Dynamic interrupt handler registration not yet implemented\n");
}

void hal_interrupt_enable(uint8_t interrupt) {
    if (interrupt >= 32 && interrupt <= 47) {
        /* Hardware IRQ */
        uint8_t irq = interrupt - 32;
        x86_pic_enable_irq(irq);
        meow_printf("HAL: Enabled IRQ %u (INT %u)\n", irq, interrupt);
    } else {
        meow_printf("HAL: Cannot enable interrupt %u (not a hardware IRQ)\n", interrupt);
    }
}

void hal_interrupt_disable(uint8_t interrupt) {
    if (interrupt >= 32 && interrupt <= 47) {
        /* Hardware IRQ */
        uint8_t irq = interrupt - 32;
        x86_pic_disable_irq(irq);
        meow_printf("HAL: Disabled IRQ %u (INT %u)\n", irq, interrupt);
    } else {
        meow_printf("HAL: Cannot disable interrupt %u (not a hardware IRQ)\n", interrupt);
    }
}

/* Debug Output */
void hal_debug_putc(char c) {
    /* Use existing VGA terminal output */
    extern void terminal_putchar(char c);
    terminal_putchar(c);
}

void hal_debug_puts(const char* str) {
    while (*str) {
        hal_debug_putc(*str++);
    }
}

/* x86-specific utility functions */
void x86_enable_interrupts_and_halt(void) {
    asm volatile("sti; hlt");
}

uint32_t x86_get_cr0(void) {
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    return cr0;
}

uint32_t x86_get_cr2(void) {
    uint32_t cr2;
    asm volatile("movl %%cr2, %0" : "=r"(cr2));
    return cr2;
}

uint32_t x86_get_cr3(void) {
    uint32_t cr3;
    asm volatile("movl %%cr3, %0" : "=r"(cr3));
    return cr3;
}
