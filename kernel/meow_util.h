/* kernel/meow_util.h - MeowKernel Cat-Themed Utility Functions Interface (Clean)
 *
 * Clean utility functions with ONLY cat-themed logging via meow_log()
 * NO backward compatibility functions - only the clean meow_log() interface
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_KERNEL_UTIL_H
#define MEOW_KERNEL_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "meow_error_definitions.h"

/* ============================================================================
 * UTILITY CONSTANTS AND CONFIGURATION
 * ============================================================================ */

/* String and buffer limits for safety */
#define MEOW_UTIL_MAX_STRING_LEN    1024
#define MEOW_UTIL_MAX_PRINTF_LEN    512
#define MEOW_UTIL_MAX_LINE_LEN      128

/* VGA text mode constants */
#define MEOW_VGA_WIDTH              80
#define MEOW_VGA_HEIGHT             25
#define MEOW_VGA_BUFFER             0xB8000

/* VGA color constants */
#define MEOW_VGA_BLACK              0x0
#define MEOW_VGA_BLUE               0x1
#define MEOW_VGA_GREEN              0x2
#define MEOW_VGA_CYAN               0x3
#define MEOW_VGA_RED                0x4
#define MEOW_VGA_MAGENTA            0x5
#define MEOW_VGA_BROWN              0x6
#define MEOW_VGA_LIGHT_GRAY         0x7
#define MEOW_VGA_DARK_GRAY          0x8
#define MEOW_VGA_LIGHT_BLUE         0x9
#define MEOW_VGA_LIGHT_GREEN        0xA
#define MEOW_VGA_LIGHT_CYAN         0xB
#define MEOW_VGA_LIGHT_RED          0xC
#define MEOW_VGA_LIGHT_MAGENTA      0xD
#define MEOW_VGA_YELLOW             0xE
#define MEOW_VGA_WHITE              0xF

/* Legacy compatibility constants for existing code */
#define VGA_MEMORY                  MEOW_VGA_BUFFER
#define VGA_WIDTH                   MEOW_VGA_WIDTH
#define VGA_HEIGHT                  MEOW_VGA_HEIGHT
#define VGA_COLOR_LIGHT_GREY        MEOW_VGA_LIGHT_GRAY
#define VGA_COLOR_BLACK             MEOW_VGA_BLACK
#define VGA_COLOR_LIGHT_MAGENTA     MEOW_VGA_LIGHT_MAGENTA
#define VGA_COLOR_LIGHT_CYAN        MEOW_VGA_LIGHT_CYAN
#define VGA_COLOR_LIGHT_BLUE        MEOW_VGA_LIGHT_BLUE
#define VGA_COLOR_LIGHT_GREEN       MEOW_VGA_LIGHT_GREEN
#define VGA_COLOR_LIGHT_RED         MEOW_VGA_LIGHT_RED
#define VGA_COLOR_LIGHT_BROWN       MEOW_VGA_BROWN
#define VGA_COLOR_WHITE             MEOW_VGA_WHITE
#define VGA_COLOR_YELLOW            MEOW_VGA_YELLOW
#define MAX_STRING_LEN              MEOW_UTIL_MAX_STRING_LEN

/* Multiboot constants */
#define MULTIBOOT_MAGIC             0x2BADB002

/* Legacy VGA color type */
typedef uint8_t vga_color;

/* ============================================================================
 * CAT-THEMED LOGGING SYSTEM 🐱 (CLEAN VERSION)
 * ============================================================================ */

/**
 * meow_log_level_t - Cat-themed log levels
 * 
 * Each log level corresponds to different cat vocalizations, from gentle
 * purring to alarmed screeching, making debugging more fun and memorable!
 */
typedef enum meow_log_level {
    MEOW_LOG_PURR   = 0,    /*  Extremely verbose debug info (gentle purring) */
    MEOW_LOG_MEOW   = 1,    /*  Debug information (normal cat talk) */
    MEOW_LOG_CHIRP  = 2,    /*  Informational messages (excited chirping) */
    MEOW_LOG_HISS   = 3,    /*  Warning messages (defensive hissing) */
    MEOW_LOG_YOWL   = 4,    /*  Error messages (distressed yowling) */
    MEOW_LOG_SCREECH = 5,   /*  Fatal errors (alarmed screeching) */
    MEOW_LOG_SILENT  = 6    /*  No logging (sleeping cat) */
} meow_log_level_t;

/* Cat-themed log level color mapping */
#define MEOW_LOG_COLOR_PURR         MEOW_VGA_DARK_GRAY      /* Soft and subtle */
#define MEOW_LOG_COLOR_MEOW         MEOW_VGA_LIGHT_GRAY     /* Normal cat color */
#define MEOW_LOG_COLOR_CHIRP        MEOW_VGA_LIGHT_CYAN     /* Happy and bright */
#define MEOW_LOG_COLOR_HISS         MEOW_VGA_YELLOW         /* Warning yellow */
#define MEOW_LOG_COLOR_YOWL         MEOW_VGA_LIGHT_RED      /* Distressed red */
#define MEOW_LOG_COLOR_SCREECH      MEOW_VGA_WHITE          /* Alarming white */

/* Cat-themed log level background colors */
#define MEOW_LOG_BG_NORMAL          MEOW_VGA_BLACK
#define MEOW_LOG_BG_SCREECH         MEOW_VGA_RED            /* Red background for fatal */

/* Cat-themed log level prefixes with emojis */
#define MEOW_LOG_PREFIX_PURR        "[PURR😻]"  /* Gentle, verbose */
#define MEOW_LOG_PREFIX_MEOW        "[MEOW😸]"  /* Normal debug */
#define MEOW_LOG_PREFIX_CHIRP       "[CHIRP😺]" /* Happy info */
#define MEOW_LOG_PREFIX_HISS        "[HISS😾]"  /* Warning */
#define MEOW_LOG_PREFIX_YOWL        "[YOWL🙀]"  /* Error */
#define MEOW_LOG_PREFIX_SCREECH     "[SCREECH😱]" /* Fatal */

/* Alternative ASCII prefixes for systems without emoji support */
#define MEOW_LOG_PREFIX_PURR_ASCII      "[PURR] "
#define MEOW_LOG_PREFIX_MEOW_ASCII      "[MEOW] "
#define MEOW_LOG_PREFIX_CHIRP_ASCII     "[CHIRP]"
#define MEOW_LOG_PREFIX_HISS_ASCII      "[HISS] "
#define MEOW_LOG_PREFIX_YOWL_ASCII      "[YOWL] "
#define MEOW_LOG_PREFIX_SCREECH_ASCII   "[SCREECH]"

/* ============================================================================
 * CORE LOGGING FUNCTIONS (CLEAN - NO REDUNDANT FUNCTIONS)
 * ============================================================================ */

/**
 * meow_log - THE ONLY centralized cat-themed logging function
 * @level: Log level (MEOW_LOG_PURR through MEOW_LOG_SCREECH)
 * @format: Printf-style format string
 * @...: Variable arguments
 * 
 * THE SINGLE logging function that handles ALL log output with cat-themed
 * levels, consistent formatting, coloring, and filtering.
 * 
 * Usage Examples:
 *   meow_log(MEOW_LOG_PURR, "Very detailed trace info");
 *   meow_log(MEOW_LOG_MEOW, "Debug information");
 *   meow_log(MEOW_LOG_CHIRP, "Informational message");
 *   meow_log(MEOW_LOG_HISS, "Warning message");
 *   meow_log(MEOW_LOG_YOWL, "Error message");
 *   meow_log(MEOW_LOG_SCREECH, "Fatal error message");
 */
void meow_log(meow_log_level_t level, const char* format, ...);

/**
 * meow_vlog - Logging with va_list (internal use)
 * @level: Cat-themed log level
 * @format: Format string
 * @args: Variable argument list
 */
void meow_vlog(meow_log_level_t level, const char* format, va_list args);

/* ============================================================================
 * LOGGING CONFIGURATION
 * ============================================================================ */

/**
 * meow_log_set_level - Set global cat log level
 * @level: Minimum cat log level to display
 * 
 * @return Previous log level
 */
meow_log_level_t meow_log_set_level(meow_log_level_t level);

/**
 * meow_log_get_level - Get current cat log level
 * 
 * @return Current log level
 */
meow_log_level_t meow_log_get_level(void);

/**
 * meow_log_level_to_string - Convert cat log level to string
 * @level: Cat log level to convert
 * 
 * @return Constant string describing the log level
 */
const char* meow_log_level_to_string(meow_log_level_t level);

/**
 * meow_log_enable_emojis - Enable/disable emoji prefixes
 * @enable: 1 to enable emojis, 0 for ASCII prefixes
 * 
 * @return Previous emoji setting
 */
uint8_t meow_log_enable_emojis(uint8_t enable);

/* ============================================================================
 * BASIC OUTPUT FUNCTIONS (Direct output without logging system)
 * ============================================================================ */

/**
 * meow_printf - Direct formatted print function (bypasses logging)
 * @format: Format string (printf-style)
 * @...: Variable arguments
 */
void meow_printf(const char* format, ...);

/**
 * meow_vprintf - Direct formatted print with va_list
 * @format: Format string
 * @args: Variable argument list
 */
void meow_vprintf(const char* format, va_list args);

/**
 * meow_puts - Direct string output
 * @str: String to output
 */
void meow_puts(const char* str);

/**
 * meow_putc - Direct character output
 * @c: Character to output
 */
void meow_putc(char c);

/* ============================================================================
 * SAFE MEMORY OPERATIONS
 * ============================================================================ */

void* meow_memset(void* dest, int value, size_t count);
void* meow_memcpy(void* dest, const void* src, size_t count);
void* meow_memmove(void* dest, const void* src, size_t count);
int meow_memcmp(const void* ptr1, const void* ptr2, size_t count);
void* meow_memchr(const void* ptr, int value, size_t count);

/* ============================================================================
 * SAFE STRING OPERATIONS
 * ============================================================================ */

size_t meow_strlen(const char* str);
char* meow_strcpy(char* dest, const char* src, size_t dest_size);
char* meow_strncpy(char* dest, const char* src, size_t count, size_t dest_size);
int meow_strcmp(const char* str1, const char* str2);
int meow_strncmp(const char* str1, const char* str2, size_t count);
char* meow_strchr(const char* str, int c);
char* meow_strrchr(const char* str, int c);
char* meow_strcat(char* dest, const char* src);

/* ============================================================================
 * NUMBER TO STRING CONVERSION
 * ============================================================================ */

int meow_itoa(int value, char* buffer, size_t buffer_size, int base);
int meow_utoa(unsigned int value, char* buffer, size_t buffer_size, int base);
int meow_ltoa(long value, char* buffer, size_t buffer_size, int base);
int meow_atoi(const char* str);

/* Helper functions for number conversion */
void meow_reverse_string(char* str, int length);
int meow_int_to_string(int num, char* str, int base);
int meow_uint_to_string(unsigned int num, char* str, int base);
int meow_longlong_to_string(long long num, char* str, int base);

/* ============================================================================
 * VGA DISPLAY MANAGEMENT
 * ============================================================================ */

/**
 * meow_vga_init - Initialize VGA text mode
 */
void meow_vga_init(void);

/**
 * meow_vga_clear - Clear the VGA screen
 */
void meow_vga_clear(void);

/**
 * meow_vga_set_color - Set VGA text colors
 * @foreground: Foreground color (0-15)
 * @background: Background color (0-15)
 */
void meow_vga_set_color(uint8_t foreground, uint8_t background);

/**
 * meow_vga_get_cursor - Get current cursor position
 * @x: Output pointer for column
 * @y: Output pointer for row
 */
void meow_vga_get_cursor(uint8_t* x, uint8_t* y);

/**
 * meow_vga_set_cursor - Set cursor position
 * @x: Column (0-79)
 * @y: Row (0-24)
 */
void meow_vga_set_cursor(uint8_t x, uint8_t y);

/* Legacy VGA functions for backward compatibility with existing code */
void clear_screen(void);
void set_text_color(vga_color fg, vga_color bg);
void set_cursor_position(int x, int y);
void terminal_putchar(char c);
void terminal_writestring(const char* str);
void print_hex(uint32_t value);
void print_decimal(uint32_t value);

/* ============================================================================
 * ASSERTION AND PANIC FUNCTIONS
 * ============================================================================ */

/**
 * meow_panic - System panic with message
 * @message: Panic message
 * 
 * Immediately halts the system with a panic message.
 */
void meow_panic(const char* message);

/* Cat-themed assertion that screeches on failure */
#ifdef DEBUG
#define meow_assert(condition, message) do { \
    if (!(condition)) { \
        meow_log(MEOW_LOG_SCREECH, "ASSERTION FAILED: %s at %s:%d - %s", \
                 #condition, __FILE__, __LINE__, message); \
        meow_panic("Cat assertion failure - the cat is not amused!"); \
    } \
} while(0)
#else
#define meow_assert(condition, message) ((void)0)
#endif

/* ============================================================================
 * UTILITY VALIDATION MACROS (Updated to use meow_log)
 * ============================================================================ */

#define MEOW_VALIDATE_PTR(ptr) do { \
    if (!(ptr)) { \
        meow_log(MEOW_LOG_YOWL, "Null pointer at %s:%d - the cat is not pleased!", __FILE__, __LINE__); \
        return NULL; \
    } \
} while(0)

#define MEOW_VALIDATE_PTR_RET(ptr, ret) do { \
    if (!(ptr)) { \
        meow_log(MEOW_LOG_YOWL, "Null pointer at %s:%d - the cat hisses in disapproval!", __FILE__, __LINE__); \
        return (ret); \
    } \
} while(0)

#define MEOW_VALIDATE_SIZE(size, max) do { \
    if ((size) > (max)) { \
        meow_log(MEOW_LOG_YOWL, "Size %zu exceeds maximum %zu at %s:%d - the cat's territory is too small!", \
                 (size_t)(size), (size_t)(max), __FILE__, __LINE__); \
        return NULL; \
    } \
} while(0)

/* Overflow detection macros */
#define MEOW_WILL_ADD_OVERFLOW(a, b) \
    ((a) > SIZE_MAX - (b))

#define MEOW_WILL_MUL_OVERFLOW(a, b) \
    ((a) != 0 && (b) > SIZE_MAX / (a))

/* Alignment macros */
#define MEOW_ALIGN_UP(value, align) \
    (((value) + (align) - 1) & ~((align) - 1))

#define MEOW_ALIGN_DOWN(value, align) \
    ((value) & ~((align) - 1))

#define MEOW_IS_ALIGNED(value, align) \
    (((value) & ((align) - 1)) == 0)

#endif /* MEOW_KERNEL_UTIL_H */