/* advanced/hal/arm64/hal_arm64.c - ARM64 HAL implementation */

#include "hal_arm64.h"

/* Forward declaration for kprintf */
extern void kprintf(const char* format, ...);

/* CPU Operations */
void hal_cpu_init(void) {
    kprintf("ARM64: Initializing CPU (EL%llu)\n", arm64_get_current_el());
    // TODO: Initialize MMU, exception vectors, etc.
}

void hal_cpu_halt(void) {
    arm64_halt();
}

void hal_cpu_disable_interrupts(void) {
    arm64_disable_interrupts();
}

void hal_cpu_enable_interrupts(void) {
    arm64_enable_interrupts();
}

uint32_t hal_cpu_get_flags(void) {
    uint64_t flags;
    asm volatile("mrs %0, DAIF" : "=r"(flags));
    return (uint32_t)flags;
}

/* Port I/O Operations (not applicable on ARM64) */
uint8_t hal_port_inb(uint16_t port) {
    (void)port;
    return 0; /* ARM64 uses memory-mapped I/O */
}

uint16_t hal_port_inw(uint16_t port) {
    (void)port;
    return 0;
}

uint32_t hal_port_inl(uint16_t port) {
    (void)port;
    return 0;
}

void hal_port_outb(uint16_t port, uint8_t data) {
    (void)port;
    (void)data;
    /* ARM64 uses memory-mapped I/O */
}

void hal_port_outw(uint16_t port, uint16_t data) {
    (void)port;
    (void)data;
}

void hal_port_outl(uint16_t port, uint32_t data) {
    (void)port;
    (void)data;
}

/* Memory Operations */
void hal_memory_init(void) {
    kprintf("ARM64: Initializing memory management\n");
    // TODO: Set up MMU, page tables
}

uint32_t hal_memory_get_total_size(void) {
    return 1024 * 1024 * 1024; /* 1GB default for Raspberry Pi */
}

uint32_t hal_memory_get_available_size(void) {
    return hal_memory_get_total_size() - (16 * 1024 * 1024); /* Reserve 16MB */
}

void* hal_memory_get_kernel_end(void) {
    extern char _kernel_end;
    return &_kernel_end;
}

/* Timer Operations */
void hal_timer_init(uint32_t frequency) {
    kprintf("ARM64: Initializing timer at %u Hz\n", frequency);
    arm64_timer_init(frequency);
}

void hal_timer_sleep(uint32_t milliseconds) {
    kprintf("ARM64: Sleep for %u ms (not implemented)\n", milliseconds);
    /* TODO: Implement ARM64 timer sleep */
}

/* Interrupt Operations */
void hal_interrupt_init(void) {
    kprintf("ARM64: Initializing interrupt system\n");
    arm64_gic_init();
}

void hal_interrupt_register_handler(uint8_t interrupt, void (*handler)(void)) {
    (void)interrupt;
    (void)handler;
    kprintf("ARM64: Register handler for interrupt %u (not implemented)\n", interrupt);
}

void hal_interrupt_enable(uint8_t interrupt) {
    (void)interrupt;
    kprintf("ARM64: Enable interrupt %u (not implemented)\n", interrupt);
}

void hal_interrupt_disable(uint8_t interrupt) {
    (void)interrupt;
    kprintf("ARM64: Disable interrupt %u (not implemented)\n", interrupt);
}

/* Debug Output */
void hal_debug_putc(char c) {
    /* TODO: Implement UART output for Raspberry Pi */
    (void)c;
}

void hal_debug_puts(const char* str) {
    while (*str) {
        hal_debug_putc(*str++);
    }
}

/* ARM64-specific stub implementations */
void arm64_mmu_init(void) {
    kprintf("ARM64: MMU initialization (stub)\n");
}

void arm64_gic_init(void) {
    kprintf("ARM64: GIC initialization (stub)\n");
}

void arm64_timer_init(uint32_t frequency) {
    kprintf("ARM64: Timer initialization at %u Hz (stub)\n", frequency);
}

void arm64_uart_init(void) {
    kprintf("ARM64: UART initialization (stub)\n");
}
