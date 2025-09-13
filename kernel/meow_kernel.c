/* kernel/meow_kernel.c - Main MeowKernel entry point */

#include <stddef.h>
#include <stdint.h>
#include "meow_util.h"
#include "../advanced/hal/hal.h"

/* Define our own bool type since stdbool.h might not be available */
typedef int bool;
#define true 1
#define false 0

/* VGA text buffer constants */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 800
#define VGA_HEIGHT 250

/* Hardware text mode color constants */
enum meow_vga_color {
    MEOW_VGA_COLOR_BLACK = 0,
    MEOW_VGA_COLOR_BLUE = 1,
    MEOW_VGA_COLOR_GREEN = 2,
    MEOW_VGA_COLOR_CYAN = 3,
    MEOW_VGA_COLOR_RED = 4,
    MEOW_VGA_COLOR_MAGENTA = 5,
    MEOW_VGA_COLOR_BROWN = 6,
    MEOW_VGA_COLOR_LIGHT_GREY = 7,
    MEOW_VGA_COLOR_DARK_GREY = 8,
    MEOW_VGA_COLOR_LIGHT_BLUE = 9,
    MEOW_VGA_COLOR_LIGHT_GREEN = 10,
    MEOW_VGA_COLOR_LIGHT_CYAN = 11,
    MEOW_VGA_COLOR_LIGHT_RED = 12,
    MEOW_VGA_COLOR_LIGHT_MAGENTA = 13,
    MEOW_VGA_COLOR_LIGHT_BROWN = 14,
    MEOW_VGA_COLOR_WHITE = 15,
};

static inline uint8_t meow_vga_entry_color(enum meow_vga_color fg, enum meow_vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t meow_vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t meow_terminal_row;
size_t meow_terminal_column;
uint8_t meow_terminal_color;
uint16_t* meow_terminal_buffer;

void meow_terminal_initialize(void) {
    meow_terminal_row = 0;
    meow_terminal_column = 0;
    meow_terminal_color = meow_vga_entry_color(MEOW_VGA_COLOR_LIGHT_GREEN, MEOW_VGA_COLOR_BLACK);
    meow_terminal_buffer = (uint16_t*) VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            meow_terminal_buffer[index] = meow_vga_entry(' ', meow_terminal_color);
        }
    }
}

void meow_terminal_setcolor(uint8_t color) {
    meow_terminal_color = color;
}

void meow_terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    meow_terminal_buffer[index] = meow_vga_entry(c, color);
}

void meow_terminal_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current_index = y * VGA_WIDTH + x;
            const size_t next_index = (y + 1) * VGA_WIDTH + x;
            meow_terminal_buffer[current_index] = meow_terminal_buffer[next_index];
        }
    }
    
    /* Clear the last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        meow_terminal_buffer[index] = meow_vga_entry(' ', meow_terminal_color);
    }
    
    if (meow_terminal_row > 0) {
        meow_terminal_row--;
    }
}

void terminal_putchar(char c) {
    if (c == 10) {  /* 10 is ASCII for newline - fixed the multi-character constant */
        meow_terminal_column = 0;
        if (++meow_terminal_row >= VGA_HEIGHT) {
            meow_terminal_scroll();
        }
        return;
    }
    
    meow_terminal_putentryat(c, meow_terminal_color, meow_terminal_column, meow_terminal_row);
    if (++meow_terminal_column >= VGA_WIDTH) {
        meow_terminal_column = 0;
        if (++meow_terminal_row >= VGA_HEIGHT) {
            meow_terminal_scroll();
        }
    }
}

void terminal_writestring(const char* data) {
    size_t len = meow_strlen(data);
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(data[i]);
    }
}

/* MeowKernel welcome banner */
void meow_print_banner(void) {
    meow_terminal_setcolor(meow_vga_entry_color(MEOW_VGA_COLOR_LIGHT_CYAN, MEOW_VGA_COLOR_BLACK));
    meow_printf("\n");
    meow_printf("      /\\_/\\  \n");
    meow_printf("     ( o.o ) \n");
    meow_printf("      > ^ <  \n");
    meow_printf("\n");
    
    meow_terminal_setcolor(meow_vga_entry_color(MEOW_VGA_COLOR_LIGHT_MAGENTA, MEOW_VGA_COLOR_BLACK));
    meow_printf(" ===== Welcome to MeowKernel! =====\n");
    meow_printf(" The Purr-fect Operating System Kernel\n");
    meow_printf(" Built with love for our feline friends\n");
    meow_printf("======================================\n");
    
    /* Reset to normal color */
    meow_terminal_setcolor(meow_vga_entry_color(MEOW_VGA_COLOR_LIGHT_GREY, MEOW_VGA_COLOR_BLACK));
}

/* Main kernel entry point */
void kernel_main(void) {
    /* Initialize terminal interface */
    meow_terminal_initialize();
    
    /* Print cat-themed welcome banner */
    meow_print_banner();
    
    /* Set debug level */
    meow_set_debug_level(MEOW_DEBUG_MEOW);
    
    /* Initialize Hardware Abstraction Layer */
    meow_printf("Initializing Hardware Abstraction Layer...\n");
    hal_init();
    meow_printf("\n");
    
    /* Boot success messages */
    meow_printf("MeowKernel successfully booted!\n");
    meow_printf("Hardware Abstraction Layer: Purring smoothly\n");
    meow_printf("Device Drivers: Loading...\n");
    meow_printf("Memory Management: Setting up...\n");
    meow_printf("System Calls: Preparing interface...\n");
    meow_printf("Resource Management: Starting...\n\n");
    
    /* Test HAL functionality with cat theme */
    meow_printf("Testing HAL functions:\n");
    meow_printf("Architecture: %s\n", hal_get_arch_string());
    meow_printf("Total Memory: %u bytes\n", hal_memory_get_total_size());
    meow_printf("Available Memory: %u bytes\n", hal_memory_get_available_size());
    meow_printf("CPU Flags: 0x%x\n", hal_cpu_get_flags());
    
    /* Display system information */
    meow_system_info();
    
    /* Some debug examples */
    meow_debug(MEOW_DEBUG_PURR, "This is a quiet debug message\n");
    meow_debug(MEOW_DEBUG_MEOW, "This is a normal debug message\n");
    meow_debug(MEOW_DEBUG_HISS, "This is a verbose debug message\n");
    
    /* Final status */
    meow_printf(" MeowKernel is now running and ready to purr!\n");
    meow_printf(" Ready for next development phase: Device Drivers\n");
    meow_printf(" All systems nominal - the cat is out of the bag!\n\n");
    
    /* Kernel main loop */
    meow_debug(MEOW_DEBUG_MEOW, "Entering main kernel loop - time for a cat nap!\n");
    while(1) {
        hal_cpu_halt();
    }
}
