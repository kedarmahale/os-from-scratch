#ifndef MEOW_UTIL_H
#define MEOW_UTIL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// =============================================================================
// VGA TEXT MODE DEFINITIONS
// =============================================================================

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Colors for cat-themed output
typedef enum {
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
} vga_color;

// =============================================================================
// TERMINAL FUNCTIONS
// =============================================================================

void clear_screen(void);
void set_text_color(vga_color fg, vga_color bg);
void set_cursor_position(int x, int y);
void terminal_putchar(char c);
void terminal_writestring(const char* str);

// =============================================================================
// NUMBER FORMATTING
// =============================================================================

void print_hex(uint32_t value);
void print_decimal(uint32_t value);

// =============================================================================
// CAT-THEMED PRINTF FUNCTIONS
// =============================================================================

int meow_printf(const char* format, ...);
int meow_debug(const char* format, ...);
int meow_info(const char* format, ...);
int meow_warn(const char* format, ...);
int meow_error(const char* format, ...);

// =============================================================================
// STRING UTILITIES
// =============================================================================

size_t meow_strlen(const char* str);
int meow_strcmp(const char* str1, const char* str2);
int meow_strncmp(const char* str1, const char* str2, size_t n);
char* meow_strcpy(char* dest, const char* src);
char* meow_strncpy(char* dest, const char* src, size_t n);
char* meow_strcat(char* dest, const char* src);
char* meow_strchr(const char* str, int c);
void meow_reverse_string(char* str, int length);

// =============================================================================
// MEOW_MORY UTILITIES
// =============================================================================

void* meow_memset(void* ptr, int value, size_t num);
void* meow_memcpy(void* dest, const void* src, size_t num);
void* meow_memmove(void* dest, const void* src, size_t num);
int meow_memcmp(const void* ptr1, const void* ptr2, size_t num);

// =============================================================================
// MEOW NUMBER CONVERSION
// =============================================================================

int meow_int_to_string(int num, char* str, int base);
int meow_uint_to_string(unsigned int num, char* str, int base);
int meow_longlong_to_string(long long num, char* str, int base);
int meow_atoi(const char* str);


#endif // MEOW_UTIL_H

