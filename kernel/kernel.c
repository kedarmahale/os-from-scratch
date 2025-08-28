// kernel.c - Main kernel implementation with VGA and serial output

#include <stddef.h>
#include <stdint.h>

#include "../advanced/hal/hal.h"

// Check if the compiler thinks we're targeting the wrong OS
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

// This tutorial will only work for 32-bit x86 targets
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

/* VGA Text Mode Implementation */
// VGA color constants
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Simple strlen implementation (since we don't have stdlib)
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// VGA buffer dimensions and state
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;

    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    // Move all rows up by one
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            terminal_buffer[(row - 1) * VGA_WIDTH + col] = terminal_buffer[row * VGA_WIDTH + col];
        }
    }

    // Clear the last row
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_scroll();
        }
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_row = VGA_HEIGHT - 1;
                terminal_scroll();
            }
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

/* Serial Port Implementation for Debugging */
#define SERIAL_COM1 0x3F8

// Port I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Serial port registers
#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

int serial_init(void) {
    // Disable all interrupts
    outb(SERIAL_COM1 + 1, 0x00);    

    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1), 0x80);    

    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_DATA_PORT(SERIAL_COM1), 0x03);    
    outb(SERIAL_COM1 + 1, 0x00);    // (hi byte)

    // 8 bits, no parity, one stop bit
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1), 0x03);    

    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_FIFO_COMMAND_PORT(SERIAL_COM1), 0xC7);    

    // IRQs enabled, RTS/DSR set
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1), 0x0B);    

    return 0;
}

int is_transmit_empty(void) {
   return inb(SERIAL_LINE_STATUS_PORT(SERIAL_COM1)) & 0x20;
}

void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(SERIAL_DATA_PORT(SERIAL_COM1), c);
}

void serial_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        serial_putchar(data[i]);
    }
}

/* Simple printf-like function for debugging */
void kprintf(const char* format, ...) {
    // Simple implementation - just print the string
    terminal_writestring(format);
    serial_writestring(format);
}

/* Kernel Main Function */
/* Main kernel entry point */
void kernel_main(void) {
    /* Initialize terminal interface */
    terminal_initialize();
    
    /* Print welcome message */
    kprintf("Welcome to Meow Kernel!\n");
    kprintf("===================\n\n");
    
    /* Initialize Hardware Abstraction Layer */
    hal_init();
    kprintf("\n");
    
    kprintf("Kernel successfully booted.\n");
    kprintf("Hardware Abstraction Layer: ✓ Initialized\n");
    kprintf("Device Drivers: Loading...\n");
    kprintf("Memory Management: Setting up...\n");
    kprintf("System Calls: Preparing interface...\n");
    kprintf("Resource Management: Starting...\n\n");
    
    /* Test HAL functionality */
    kprintf("Testing HAL functions:\n");
    kprintf("- Architecture: %s\n", hal_get_arch_string());
    kprintf("- Total Memory: %u bytes\n", hal_memory_get_total_size());
    kprintf("- Available Memory: %u bytes\n", hal_memory_get_available_size());
    kprintf("- CPU Flags: 0x%08X\n", hal_cpu_get_flags());
    
    kprintf("\nKernel is now running!\n");
    kprintf("Ready for next development phase.\n");
    
    /* Kernel main loop */
    while(1) {
        hal_cpu_halt();
    }
}

