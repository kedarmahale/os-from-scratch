/* kernel/meow_util.c - MeowKernel Cat-Themed Utility Functions Implementation (Clean)
 *
 * Clean implementation with ONLY meow_log() - NO redundant logging functions
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_util.h"

/* ============================================================================
 * GLOBAL STATE FOR VGA AND LOGGING
 * ============================================================================ */

/* VGA display state */
static uint16_t* vga_buffer = (uint16_t*)MEOW_VGA_BUFFER;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_fg = MEOW_VGA_LIGHT_GRAY;
static uint8_t current_bg = MEOW_VGA_BLACK;

/* Cat-themed logging state */
static meow_log_level_t current_log_level = MEOW_LOG_CHIRP;  /* Default: show info and above */
static uint8_t emojis_enabled = 1;  /* Enable emojis by default */

/* ============================================================================
 * VGA HELPER FUNCTIONS
 * ============================================================================ */

/* Helper function to make VGA entry */
static inline uint16_t vga_entry(unsigned char uc, uint8_t fg, uint8_t bg) {
    return uc | (uint16_t)fg << 8 | (uint16_t)bg << 12;
}

/* Scroll screen up by one line */
static void scroll_up(void) {
    /* Move all lines up */
    for (size_t y = 0; y < MEOW_VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < MEOW_VGA_WIDTH; x++) {
            vga_buffer[y * MEOW_VGA_WIDTH + x] = 
                vga_buffer[(y + 1) * MEOW_VGA_WIDTH + x];
        }
    }

    /* Clear the last line */
    for (size_t x = 0; x < MEOW_VGA_WIDTH; x++) {
        vga_buffer[(MEOW_VGA_HEIGHT - 1) * MEOW_VGA_WIDTH + x] = 
            vga_entry(' ', current_fg, current_bg);
    }

    cursor_y = MEOW_VGA_HEIGHT - 1;
}

/* ============================================================================
 * CAT-THEMED LOGGING IMPLEMENTATION
 * ============================================================================ */

/**
 * Get cat-themed prefix for log level
 */
static const char* get_cat_prefix(meow_log_level_t level) {
    if (emojis_enabled) {
        switch (level) {
            case MEOW_LOG_PURR:     return MEOW_LOG_PREFIX_PURR;
            case MEOW_LOG_MEOW:     return MEOW_LOG_PREFIX_MEOW;
            case MEOW_LOG_CHIRP:    return MEOW_LOG_PREFIX_CHIRP;
            case MEOW_LOG_HISS:     return MEOW_LOG_PREFIX_HISS;
            case MEOW_LOG_YOWL:     return MEOW_LOG_PREFIX_YOWL;
            case MEOW_LOG_SCREECH:  return MEOW_LOG_PREFIX_SCREECH;
            default:                return "[UNKNOWN😿]";
        }
    } else {
        switch (level) {
            case MEOW_LOG_PURR:     return MEOW_LOG_PREFIX_PURR_ASCII;
            case MEOW_LOG_MEOW:     return MEOW_LOG_PREFIX_MEOW_ASCII;
            case MEOW_LOG_CHIRP:    return MEOW_LOG_PREFIX_CHIRP_ASCII;
            case MEOW_LOG_HISS:     return MEOW_LOG_PREFIX_HISS_ASCII;
            case MEOW_LOG_YOWL:     return MEOW_LOG_PREFIX_YOWL_ASCII;
            case MEOW_LOG_SCREECH:  return MEOW_LOG_PREFIX_SCREECH_ASCII;
            default:                return "[UNKNOWN]";
        }
    }
}

/**
 * Get color for cat-themed log level
 */
static uint8_t get_cat_color(meow_log_level_t level) {
    switch (level) {
        case MEOW_LOG_PURR:     return MEOW_LOG_COLOR_PURR;
        case MEOW_LOG_MEOW:     return MEOW_LOG_COLOR_MEOW;
        case MEOW_LOG_CHIRP:    return MEOW_LOG_COLOR_CHIRP;
        case MEOW_LOG_HISS:     return MEOW_LOG_COLOR_HISS;
        case MEOW_LOG_YOWL:     return MEOW_LOG_COLOR_YOWL;
        case MEOW_LOG_SCREECH:  return MEOW_LOG_COLOR_SCREECH;
        default:                return MEOW_VGA_LIGHT_GRAY;
    }
}

/**
 * Get background color for cat-themed log level
 */
static uint8_t get_cat_bg_color(meow_log_level_t level) {
    switch (level) {
        case MEOW_LOG_SCREECH:  return MEOW_LOG_BG_SCREECH;  /* Red background for fatal */
        default:                return MEOW_LOG_BG_NORMAL;   /* Normal background */
    }
}

/**
 * THE ONLY cat-themed logging implementation - meow_vlog
 */
void meow_vlog(meow_log_level_t level, const char* format, va_list args) {
    /* Filter based on current log level */
    if (level < current_log_level) {
        return;
    }

    /* Save current colors */
    uint8_t saved_fg = current_fg;
    uint8_t saved_bg = current_bg;

    /* Set cat-themed colors */
    current_fg = get_cat_color(level);
    current_bg = get_cat_bg_color(level);

    /* Print cat-themed prefix */
    const char* prefix = get_cat_prefix(level);
    terminal_writestring(prefix);
    terminal_writestring(" ");

    /* Print the formatted message */
    char buffer[MEOW_UTIL_MAX_PRINTF_LEN];
    int len = 0;

    /* Simple printf implementation */
    for (int i = 0; format[i] != '\0' && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; i++) {
        if (format[i] != '%') {
            buffer[len++] = format[i];
        } else {
            i++; /* Move past '%' */
            switch (format[i]) {
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    while (*str && len < MEOW_UTIL_MAX_PRINTF_LEN - 1) {
                        buffer[len++] = *str++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    char temp[16];
                    int temp_len = meow_int_to_string(num, temp, 10);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 10);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        char c = temp[j];
                        if (c >= 'A' && c <= 'F') c = c - 'A' + 'a';
                        buffer[len++] = c;
                    }
                    break;
                }
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 16);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    buffer[len++] = c;
                    break;
                }
                case '%': {
                    buffer[len++] = '%';
                    break;
                }
                default: {
                    /* Unsupported format specifier */
                    buffer[len++] = '%';
                    if (len < MEOW_UTIL_MAX_PRINTF_LEN - 1) {
                        buffer[len++] = format[i];
                    }
                    break;
                }
            }
        }
    }

    buffer[len] = '\0';
    terminal_writestring(buffer);
    terminal_writestring("\n");

    /* Restore colors */
    current_fg = saved_fg;
    current_bg = saved_bg;
}

/**
 * THE ONLY cat-themed logging function - meow_log
 * This is the SINGLE point of entry for ALL logging in the kernel
 */
void meow_log(meow_log_level_t level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    meow_vlog(level, format, args);
    va_end(args);
}

/* ============================================================================
 * LOGGING CONFIGURATION FUNCTIONS
 * ============================================================================ */

meow_log_level_t meow_log_set_level(meow_log_level_t level) {
    meow_log_level_t previous = current_log_level;
    current_log_level = level;
    return previous;
}

meow_log_level_t meow_log_get_level(void) {
    return current_log_level;
}

const char* meow_log_level_to_string(meow_log_level_t level) {
    switch (level) {
        case MEOW_LOG_PURR:     return "PURR (Extremely verbose)";
        case MEOW_LOG_MEOW:     return "MEOW (Debug information)";
        case MEOW_LOG_CHIRP:    return "CHIRP (Information)";
        case MEOW_LOG_HISS:     return "HISS (Warning)";
        case MEOW_LOG_YOWL:     return "YOWL (Error)";
        case MEOW_LOG_SCREECH:  return "SCREECH (Fatal)";
        case MEOW_LOG_SILENT:   return "SILENT (No logging)";
        default:                return "UNKNOWN";
    }
}

uint8_t meow_log_enable_emojis(uint8_t enable) {
    uint8_t previous = emojis_enabled;
    emojis_enabled = enable;
    return previous;
}

/* ============================================================================
 * VGA DISPLAY FUNCTIONS
 * ============================================================================ */

void meow_vga_init(void) {
    vga_buffer = (uint16_t*)MEOW_VGA_BUFFER;
    cursor_x = 0;
    cursor_y = 0;
    current_fg = MEOW_VGA_LIGHT_GRAY;
    current_bg = MEOW_VGA_BLACK;
}

void meow_vga_clear(void) {
    for (size_t y = 0; y < MEOW_VGA_HEIGHT; y++) {
        for (size_t x = 0; x < MEOW_VGA_WIDTH; x++) {
            const size_t index = y * MEOW_VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', current_fg, current_bg);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void meow_vga_set_color(uint8_t foreground, uint8_t background) {
    current_fg = foreground;
    current_bg = background;
}

void meow_vga_get_cursor(uint8_t* x, uint8_t* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void meow_vga_set_cursor(uint8_t x, uint8_t y) {
    if (x < MEOW_VGA_WIDTH) cursor_x = x;
    if (y < MEOW_VGA_HEIGHT) cursor_y = y;
}

/* ============================================================================
 * LEGACY VGA FUNCTIONS (For existing code compatibility)
 * ============================================================================ */

void clear_screen(void) {
    meow_vga_clear();
}

void set_text_color(vga_color fg, vga_color bg) {
    meow_vga_set_color(fg, bg);
}

void set_cursor_position(int x, int y) {
    meow_vga_set_cursor(x, y);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~(8 - 1);
    } else if (c >= ' ') {
        const size_t index = cursor_y * MEOW_VGA_WIDTH + cursor_x;
        vga_buffer[index] = vga_entry(c, current_fg, current_bg);
        cursor_x++;
    }

    if (cursor_x >= MEOW_VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= MEOW_VGA_HEIGHT) {
        scroll_up();
    }
}

void terminal_writestring(const char* str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}

void print_hex(uint32_t value) {
    terminal_writestring("0x");
    char hex_chars[] = "0123456789ABCDEF";
    char hex_str[9] = {0};
    for (int i = 7; i >= 0; i--) {
        hex_str[7-i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    terminal_writestring(hex_str);
}

void print_decimal(uint32_t value) {
    if (value == 0) {
        terminal_putchar('0');
        return;
    }

    char buffer[12];
    int pos = 0;
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }

    /* Print in reverse order */
    for (int i = pos - 1; i >= 0; i--) {
        terminal_putchar(buffer[i]);
    }
}

/* ============================================================================
 * DIRECT OUTPUT FUNCTIONS (Bypass logging system)
 * ============================================================================ */

void meow_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    meow_vprintf(format, args);
    va_end(args);
}

void meow_vprintf(const char* format, va_list args) {
    char buffer[MEOW_UTIL_MAX_PRINTF_LEN];
    int len = 0;

    /* Simple printf implementation */
    for (int i = 0; format[i] != '\0' && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; i++) {
        if (format[i] != '%') {
            buffer[len++] = format[i];
        } else {
            i++; /* Move past '%' */
            switch (format[i]) {
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    while (*str && len < MEOW_UTIL_MAX_PRINTF_LEN - 1) {
                        buffer[len++] = *str++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    char temp[16];
                    int temp_len = meow_int_to_string(num, temp, 10);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 10);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        char c = temp[j];
                        if (c >= 'A' && c <= 'F') c = c - 'A' + 'a';
                        buffer[len++] = c;
                    }
                    break;
                }
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    char temp[16];
                    int temp_len = meow_uint_to_string(num, temp, 16);
                    for (int j = 0; j < temp_len && len < MEOW_UTIL_MAX_PRINTF_LEN - 1; j++) {
                        buffer[len++] = temp[j];
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    buffer[len++] = c;
                    break;
                }
                case '%': {
                    buffer[len++] = '%';
                    break;
                }
                default: {
                    /* Unsupported format specifier */
                    buffer[len++] = '%';
                    if (len < MEOW_UTIL_MAX_PRINTF_LEN - 1) {
                        buffer[len++] = format[i];
                    }
                    break;
                }
            }
        }
    }

    buffer[len] = '\0';
    terminal_writestring(buffer);
}

void meow_puts(const char* str) {
    if (str) {
        terminal_writestring(str);
        terminal_writestring("\n");
    }
}

void meow_putc(char c) {
    terminal_putchar(c);
}

/* ============================================================================
 * MEMORY OPERATIONS
 * ============================================================================ */

void* meow_memset(void* dest, int value, size_t count) {
    if (!dest || count == 0) {
        return dest;
    }

    unsigned char* d = (unsigned char*)dest;
    unsigned char v = (unsigned char)value;
    while (count--) {
        *d++ = v;
    }

    return dest;
}

void* meow_memcpy(void* dest, const void* src, size_t count) {
    if (!dest || !src || count == 0) {
        return dest;
    }

    if (dest == src) {
        return dest;
    }

    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) {
        *d++ = *s++;
    }

    return dest;
}

void* meow_memmove(void* dest, const void* src, size_t count) {
    if (!dest || !src || count == 0 || dest == src) {
        return dest;
    }

    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    if (d < s) {
        /* Copy forward */
        while (count--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        /* Copy backward */
        d += count;
        s += count;
        while (count--) {
            *(--d) = *(--s);
        }
    }

    return dest;
}

int meow_memcmp(const void* ptr1, const void* ptr2, size_t count) {
    if (!ptr1 || !ptr2) {
        return (ptr1 == ptr2) ? 0 : (ptr1 ? 1 : -1);
    }

    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void* meow_memchr(const void* ptr, int value, size_t count) {
    if (!ptr) {
        return NULL;
    }

    const unsigned char* p = (const unsigned char*)ptr;
    unsigned char v = (unsigned char)value;
    while (count--) {
        if (*p == v) {
            return (void*)p;
        }
        p++;
    }
    return NULL;
}

/* ============================================================================
 * STRING OPERATIONS
 * ============================================================================ */

size_t meow_strlen(const char* str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] && len < MEOW_UTIL_MAX_STRING_LEN) {
        len++;
    }
    return len;
}

int meow_strcmp(const char* str1, const char* str2) {
    if (!str1 || !str2) {
        return (str1 == str2) ? 0 : (str1 ? 1 : -1);
    }
    
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int meow_strncmp(const char* str1, const char* str2, size_t count) {
    if (!str1 || !str2 || count == 0) {
        return (str1 == str2) ? 0 : (str1 ? 1 : -1);
    }

    while (count && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        count--;
    }
    return (count == 0) ? 0 : (*(unsigned char*)str1 - *(unsigned char*)str2);
}

char* meow_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return dest;
    }

    char* orig_dest = dest;
    size_t i = 0;
    while (i < dest_size - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return orig_dest;
}

char* meow_strncpy(char* dest, const char* src, size_t count, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return dest;
    }

    char* orig_dest = dest;
    size_t i = 0;
    size_t max_copy = (count < dest_size - 1) ? count : dest_size - 1;
    
    while (i < max_copy && src[i]) {
        dest[i] = src[i];
        i++;
    }
    
    /* Null-terminate */
    dest[i] = '\0';
    
    return orig_dest;
}

char* meow_strchr(const char* str, int c) {
    if (!str) return NULL;
    
    while (*str) {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return (*str == c) ? (char*)str : NULL;
}

char* meow_strrchr(const char* str, int c) {
    if (!str) return NULL;
    
    const char* last = NULL;
    while (*str) {
        if (*str == c) {
            last = str;
        }
        str++;
    }
    return (char*)last;
}

char* meow_strcat(char* dest, const char* src) {
    if (!dest || !src) return dest;
    
    char* orig_dest = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return orig_dest;
}

/* ============================================================================
 * NUMBER CONVERSION FUNCTIONS
 * ============================================================================ */

void meow_reverse_string(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int meow_int_to_string(int num, char* str, int base) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        num = num / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    meow_reverse_string(str, i);
    return i;
}

int meow_uint_to_string(unsigned int num, char* str, int base) {
    int i = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
        num = num / base;
    }

    str[i] = '\0';
    meow_reverse_string(str, i);
    return i;
}

int meow_longlong_to_string(long long num, char* str, int base) {
    /* For simplicity, cast to int if it fits to avoid 64-bit division */
    if (num >= INT32_MIN && num <= INT32_MAX) {
        return meow_int_to_string((int)num, str, base);
    }
    
    /* For larger numbers, simplified implementation */
    return meow_int_to_string((int)num, str, base);
}

int meow_atoi(const char* str) {
    if (!str) return 0;
    
    int result = 0;
    int sign = 1;

    /* Skip whitespace */
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }

    /* Handle sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    /* Convert digits */
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

int meow_itoa(int value, char* buffer, size_t buffer_size, int base) {
    if (!buffer || buffer_size < 2) return -1;
    return meow_int_to_string(value, buffer, base);
}

int meow_utoa(unsigned int value, char* buffer, size_t buffer_size, int base) {
    if (!buffer || buffer_size < 2) return -1;
    return meow_uint_to_string(value, buffer, base);
}

int meow_ltoa(long value, char* buffer, size_t buffer_size, int base) {
    if (!buffer || buffer_size < 2) return -1;
    return meow_int_to_string((int)value, buffer, base);
}

/* ============================================================================
 * PANIC AND ERROR FUNCTIONS
 * ============================================================================ */

void meow_panic(const char* message) {
    /* Save current colors */
    uint8_t saved_fg = current_fg;
    uint8_t saved_bg = current_bg;
    
    /* Set panic colors (white on red) */
    set_text_color(MEOW_VGA_WHITE, MEOW_VGA_RED);
    clear_screen();
    
    terminal_writestring("\n\n");
    terminal_writestring("  ========== MEOWKERNEL PANIC - CATS ARE VERY UNHAPPY! ==========\n\n");
    terminal_writestring("   CATASTROPHIC ERROR - The cats have encountered a serious problem!\n\n");
    terminal_writestring("  Reason: ");
    if (message) {
        terminal_writestring(message);
    } else {
        terminal_writestring("Unknown cat catastrophe");
    }
    terminal_writestring("\n\n");
    terminal_writestring("   The cats have decided to halt the system to prevent further\n");
    terminal_writestring("     damage. Please check your code and restart the system.\n\n");
    terminal_writestring("   System halted. Press reset to restart.\n\n");
    terminal_writestring("  ============================================================\n");
    
    /* Halt the system */
    while (1) {
        asm volatile("hlt");
    }
}

/* ============================================================================
 * ERROR UTILITY FUNCTIONS
 * ============================================================================ */

const char* meow_error_to_string(meow_error_t error) {
    switch (error) {
        case MEOW_SUCCESS:                      return "Success - The cat is happy!";
        case MEOW_ERROR_GENERAL:                return "General error - The cat is confused";
        case MEOW_ERROR_UNKNOWN:                return "Unknown error - The cat doesn't understand";
        case MEOW_ERROR_NULL_POINTER:           return "Null pointer - The cat found nothing";
        case MEOW_ERROR_INVALID_PARAMETER:      return "Invalid parameter - The cat is not pleased";
        case MEOW_ERROR_INVALID_SIZE:           return "Invalid size - Wrong size for the cat";
        case MEOW_ERROR_INVALID_ALIGNMENT:      return "Invalid alignment - The cat wants things tidy";
        case MEOW_ERROR_BUFFER_TOO_SMALL:       return "Buffer too small - Not enough space for the cat";
        case MEOW_ERROR_INVALID_STATE:          return "Invalid state - The cat is in the wrong mood";
        case MEOW_ERROR_OUT_OF_MEMORY:          return "Out of memory - The cat needs more space";
        case MEOW_ERROR_MEMORY_CORRUPTION:      return "Memory corruption - Something damaged the cat's territory";
        case MEOW_ERROR_DOUBLE_FREE:            return "Double free - The cat already left that space";
        case MEOW_ERROR_HEAP_EXHAUSTED:         return "Heap exhausted - No more cozy spots for cats";
        case MEOW_ERROR_HARDWARE_FAILURE:       return "Hardware failure - The cat's equipment is broken";
        case MEOW_ERROR_NOT_INITIALIZED:        return "Not initialized - The cat hasn't set up yet";
        case MEOW_ERROR_ALREADY_INITIALIZED:    return "Already initialized - The cat is already ready";
        case MEOW_ERROR_INITIALIZATION_FAILED:  return "Initialization failed - The cat couldn't get ready";
        case MEOW_ERROR_TIMEOUT:                return "Timeout - The cat got impatient";
        case MEOW_ERROR_NOT_SUPPORTED:          return "Not supported - The cat doesn't know how to do that";
        case MEOW_ERROR_ACCESS_DENIED:          return "Access denied - The cat won't let you";
        default:                                return "Unknown error code - The cat is very confused";
    }
}

const char* meow_error_get_category(meow_error_t error) {
    if (MEOW_IS_PARAM_ERROR(error))         return "Parameter";
    if (MEOW_IS_MEMORY_ERROR(error))        return "Memory";
    if (MEOW_IS_HARDWARE_ERROR(error))      return "Hardware";
    if (MEOW_IS_SYSTEM_ERROR(error))        return "System";
    if (MEOW_IS_IO_ERROR(error))            return "I/O";
    return "Unknown";
}

uint8_t meow_error_is_recoverable(meow_error_t error) {
    switch (error) {
        case MEOW_ERROR_TIMEOUT:
        case MEOW_ERROR_DEVICE_BUSY:
        case MEOW_ERROR_RESOURCE_EXHAUSTED:
        case MEOW_ERROR_IO_FAILURE:
            return 1;  /* These might be recoverable */
        
        case MEOW_ERROR_MEMORY_CORRUPTION:
        case MEOW_ERROR_HARDWARE_FAILURE:
        case MEOW_ERROR_INITIALIZATION_FAILED:
            return 0;  /* These are likely fatal */
        
        default:
            return 1;  /* Default to recoverable */
    }
}