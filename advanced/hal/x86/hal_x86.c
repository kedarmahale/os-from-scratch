#include "hal_x86.h"

/* Forward declaration for kprintf */
extern void kprintf(const char* format, ...);

/* Port I/O Implementation - AT&T syntax */
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
    kprintf("HAL: Initializing x86 CPU\n");
    x86_gdt_init();
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
    kprintf("HAL: Initializing x86 memory management\n");
}

uint32_t hal_memory_get_total_size(void) {
    return 128 * 1024 * 1024; // 128MB default
}

uint32_t hal_memory_get_available_size(void) {
    return hal_memory_get_total_size() - (4 * 1024 * 1024); // Reserve 4MB for kernel
}

void* hal_memory_get_kernel_end(void) {
    extern char _kernel_end;
    return &_kernel_end;
}

/* Timer Operations */
void hal_timer_init(uint32_t frequency) {
    kprintf("HAL: Initializing timer at %u Hz\n", frequency);
    x86_pit_init(frequency);
}

void hal_timer_sleep(uint32_t milliseconds) {
    uint64_t start_ticks = hal_timer_get_ticks();
    uint64_t end_ticks = start_ticks + milliseconds;
    
    while (hal_timer_get_ticks() < end_ticks) {
        hal_cpu_halt();
    }
}

/* Interrupt Operations */
void hal_interrupt_init(void) {
    kprintf("HAL: Initializing x86 interrupts\n");
    x86_idt_init();
    x86_pic_init();
}

void hal_interrupt_register_handler(uint8_t interrupt, void (*handler)(void)) {
    kprintf("HAL: Registering handler for interrupt %u\n", interrupt);
}

void hal_interrupt_enable(uint8_t interrupt) {
    kprintf("HAL: Enabling interrupt %u\n", interrupt);
}

void hal_interrupt_disable(uint8_t interrupt) {
    kprintf("HAL: Disabling interrupt %u\n", interrupt);
}

/* Debug Output */
void hal_debug_putc(char c) {
    extern void terminal_putchar(char c);
    terminal_putchar(c);
}

void hal_debug_puts(const char* str) {
    while (*str) {
        hal_debug_putc(*str++);
    }
}

/* Placeholder implementations */
void x86_gdt_init(void) {
    kprintf("x86: GDT initialization (placeholder)\n");
}

void x86_idt_init(void) {
    kprintf("x86: IDT initialization (placeholder)\n");
}

void x86_pic_init(void) {
    kprintf("x86: PIC initialization (placeholder)\n");
}

void x86_pit_init(uint32_t frequency) {
    kprintf("x86: PIT initialization at %u Hz (placeholder)\n", frequency);
}


