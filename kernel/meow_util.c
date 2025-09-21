#include "meow_util.h"

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static vga_color current_fg = VGA_COLOR_LIGHT_GREY;
static vga_color current_bg = VGA_COLOR_BLACK;

// Helper function to make VGA entry
static inline uint16_t vga_entry(unsigned char uc, vga_color fg, vga_color bg) {
    return uc | (uint16_t)fg << 8 | (uint16_t)bg << 12;
}

// Clear the screen
void clear_screen(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', current_fg, current_bg);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

// Set text color
void set_text_color(vga_color fg, vga_color bg) {
    current_fg = fg;
    current_bg = bg;
}

// Set cursor position
void set_cursor_position(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

// Scroll screen up by one line
static void scroll_up(void) {
    // Move all lines up
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_fg, current_bg);
    }
    
    cursor_y = VGA_HEIGHT - 1;
}

// Print a single character
void terminal_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~(8 - 1);
    } else if (c >= ' ') {
        const size_t index = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[index] = vga_entry(c, current_fg, current_bg);
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        scroll_up();
    }
}

// Print a string
void terminal_writestring(const char* str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}


// Print hexadecimal number
void print_hex(uint32_t value) {
    terminal_writestring("0x");
    char hex_chars[] = "0123456789ABCDEF";
    char hex_str[9] = {0};
    
    for (int i = 7; i >= 0; i--) {
        hex_str[7-i] = hex_chars[(value >> (i * 4)) & 0xF];
    }
    terminal_writestring(hex_str);
}

// Print decimal number
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
    
    // Print in reverse order
    for (int i = pos - 1; i >= 0; i--) {
        terminal_putchar(buffer[i]);
    }
}

/* ================================
 * MEOW STRING UTILITIES
 * ================================ */

// String length
//size_t strlen(const char* str) {
//    size_t len = 0;
//    while (str[len]) len++;
//    return len;
//}

/* Calculate string length */
size_t meow_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/* Compare two strings */
int meow_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

/* Compare first n characters of two strings */
int meow_strncmp(const char* str1, const char* str2, size_t n) {
    while (n && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    return (n == 0) ? 0 : (*(unsigned char*)str1 - *(unsigned char*)str2);
}

/* Copy string */
char* meow_strcpy(char* dest, const char* src) {
    char* orig_dest = dest;
    while ((*dest++ = *src++));
    return orig_dest;
}

/* Copy at most n characters */
char* meow_strncpy(char* dest, const char* src, size_t n) {
    char* orig_dest = dest;
    while (n && (*dest++ = *src++)) {
        n--;
    }
    while (n--) {
        *dest++ = '\0';
    }
    return orig_dest;
}

/* Concatenate strings */
char* meow_strcat(char* dest, const char* src) {
    char* orig_dest = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return orig_dest;
}

/* Find character in string */
char* meow_strchr(const char* str, int c) {
    while (*str) {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return (*str == c) ? (char*)str : NULL;
}

/* ================================
 *  MEOW-MORY UTILITIES
 * ================================ */

/* Set memory to a specific value */
void* meow_memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

/* Copy memory */
void* meow_memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

/* Move memory (handles overlapping regions) */
void* meow_memmove(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        /* Copy forward */
        while (num--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        /* Copy backward */
        d += num;
        s += num;
        while (num--) {
            *(--d) = *(--s);
        }
    }
    return dest;
}

/* Compare memory */
int meow_memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/* ================================
 *  MEOW NUMBER CONVERSION
 * ================================ */

/* Reverse a string */
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

/* Convert integer to string */
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

/* Convert unsigned integer to string */
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

/* Convert long long to string */
/* Simplified version that avoids 64-bit division */
int meow_longlong_to_string(long long num, char* str, int base) {
    /* For very large numbers, we can split into two parts */
    /* Or cast to int if we know the range is acceptable */
    
    if (num >= INT_MIN && num <= INT_MAX) {
        /* Use the regular int function for numbers that fit */
        return meow_int_to_string((int)num, str, base);
    }
    
    /* For larger numbers, implement without native 64-bit division */
    /* This is more complex but avoids the libgcc dependency */
    
    // Simplified implementation for now
    return meow_int_to_string((int)num, str, base);
}

/* Convert string to integer */
int meow_atoi(const char* str) {
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

/* Main meow_printf function with format specifier support */
int meow_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    char buffer[64];
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            chars_written++;
        } else {
            i++; /* Move past '%' */
            
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    
                    terminal_writestring(str);
                    chars_written += meow_strlen(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    int len = meow_int_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                   }
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    uintptr_t addr = (uintptr_t)ptr;
                    terminal_writestring("0x");
                    int len = meow_uint_to_string(addr, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                    }
                    terminal_writestring(buffer);
                    chars_written += len + 2;
                    break;
                }
                
                case 'l': {
                    i++; /* Check next character */
                    if (format[i] == 'l') {
                        /* long long */
                        i++;
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long long num = va_arg(args, long long);
                                int len = meow_longlong_to_string(num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                int len = meow_longlong_to_string((long long)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                /* Unsupported */
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 4;
                                break;
                        }
                    } else {
                        /* single long */
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long num = va_arg(args, long);
                                int len = meow_int_to_string((int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                int len = meow_uint_to_string((unsigned int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 3;
                                break;
                        }
                    }
                    break;
                }
                
                case '%': {
                    terminal_putchar('%');
                    chars_written++;
                    break;
                }
                
                default: {
                    /* Unsupported format specifier */
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    chars_written += 2;
                    break;
                }
            }
        }
    }
    
    va_end(args);
    return chars_written;
}


int meow_debug(const char* format, ...) {
    set_text_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_writestring("MEOW DEBUG: ");
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    char buffer[64];
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            chars_written++;
        } else {
            i++; /* Move past '%' */
            
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    
                    terminal_writestring(str);
                    chars_written += meow_strlen(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    int len = meow_int_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                   }
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    uintptr_t addr = (uintptr_t)ptr;
                    terminal_writestring("0x");
                    int len = meow_uint_to_string(addr, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                    }
                    terminal_writestring(buffer);
                    chars_written += len + 2;
                    break;
                }
                
                case 'l': {
                    i++; /* Check next character */
                    if (format[i] == 'l') {
                        /* long long */
                        i++;
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long long num = va_arg(args, long long);
                                int len = meow_longlong_to_string(num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                int len = meow_longlong_to_string((long long)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                /* Unsupported */
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 4;
                                break;
                        }
                    } else {
                        /* single long */
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long num = va_arg(args, long);
                                int len = meow_int_to_string((int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                int len = meow_uint_to_string((unsigned int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 3;
                                break;
                        }
                    }
                    break;
                }
                
                case '%': {
                    terminal_putchar('%');
                    chars_written++;
                    break;
                }
                
                default: {
                    /* Unsupported format specifier */
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    chars_written += 2;
                    break;
                }
            }
        }
    }
    
    va_end(args);
    terminal_writestring("\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    return chars_written;
}

int meow_info(const char* format, ...) {
    set_text_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("MEOW INFO: ");
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    char buffer[64];
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            chars_written++;
        } else {
            i++; /* Move past '%' */
            
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    
                    terminal_writestring(str);
                    chars_written += meow_strlen(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    int len = meow_int_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                   }
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    uintptr_t addr = (uintptr_t)ptr;
                    terminal_writestring("0x");
                    int len = meow_uint_to_string(addr, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                    }
                    terminal_writestring(buffer);
                    chars_written += len + 2;
                    break;
                }
                
                case 'l': {
                    i++; /* Check next character */
                    if (format[i] == 'l') {
                        /* long long */
                        i++;
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long long num = va_arg(args, long long);
                                int len = meow_longlong_to_string(num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                int len = meow_longlong_to_string((long long)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                /* Unsupported */
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 4;
                                break;
                        }
                    } else {
                        /* single long */
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long num = va_arg(args, long);
                                int len = meow_int_to_string((int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                int len = meow_uint_to_string((unsigned int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 3;
                                break;
                        }
                    }
                    break;
                }
                
                case '%': {
                    terminal_putchar('%');
                    chars_written++;
                    break;
                }
                
                default: {
                    /* Unsupported format specifier */
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    chars_written += 2;
                    break;
                }
            }
        }
    }
    
    va_end(args);
    terminal_writestring("\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    return chars_written;
}


int meow_warn(const char* format, ...) {
    set_text_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    terminal_writestring("MEOW WARN: ");
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    char buffer[64];
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            chars_written++;
        } else {
            i++; /* Move past '%' */
            
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    
                    terminal_writestring(str);
                    chars_written += meow_strlen(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    int len = meow_int_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                   }
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    uintptr_t addr = (uintptr_t)ptr;
                    terminal_writestring("0x");
                    int len = meow_uint_to_string(addr, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                    }
                    terminal_writestring(buffer);
                    chars_written += len + 2;
                    break;
                }
                
                case 'l': {
                    i++; /* Check next character */
                    if (format[i] == 'l') {
                        /* long long */
                        i++;
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long long num = va_arg(args, long long);
                                int len = meow_longlong_to_string(num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                int len = meow_longlong_to_string((long long)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                /* Unsupported */
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 4;
                                break;
                        }
                    } else {
                        /* single long */
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long num = va_arg(args, long);
                                int len = meow_int_to_string((int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                int len = meow_uint_to_string((unsigned int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 3;
                                break;
                        }
                    }
                    break;
                }
                
                case '%': {
                    terminal_putchar('%');
                    chars_written++;
                    break;
                }
                
                default: {
                    /* Unsupported format specifier */
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    chars_written += 2;
                    break;
                }
            }
        }
    }
    
    va_end(args);
    terminal_writestring("\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    return chars_written;
}


int meow_error(const char* format, ...) {
    set_text_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_writestring("MEOW ERROR: ");
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    char buffer[64];
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            terminal_putchar(format[i]);
            chars_written++;
        } else {
            i++; /* Move past '%' */
            
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': {
                    char* str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    
                    terminal_writestring(str);
                    chars_written += meow_strlen(str);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int num = va_arg(args, int);
                    int len = meow_int_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 10);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                   }
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'X': {
                    unsigned int num = va_arg(args, unsigned int);
                    int len = meow_uint_to_string(num, buffer, 16);
                    terminal_writestring(buffer);
                    chars_written += len;
                    break;
                }
                
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    uintptr_t addr = (uintptr_t)ptr;
                    terminal_writestring("0x");
                    int len = meow_uint_to_string(addr, buffer, 16);
                    /* Convert to lowercase */
                    for (int j = 0; j < len; j++) {
                        if (buffer[j] >= 'A' && buffer[j] <= 'F') {
                            buffer[j] = buffer[j] - 'A' + 'a';
                        }
                    }
                    terminal_writestring(buffer);
                    chars_written += len + 2;
                    break;
                }
                
                case 'l': {
                    i++; /* Check next character */
                    if (format[i] == 'l') {
                        /* long long */
                        i++;
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long long num = va_arg(args, long long);
                                int len = meow_longlong_to_string(num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                int len = meow_longlong_to_string((long long)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                /* Unsupported */
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 4;
                                break;
                        }
                    } else {
                        /* single long */
                        switch (format[i]) {
                            case 'd':
                            case 'i': {
                                long num = va_arg(args, long);
                                int len = meow_int_to_string((int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                int len = meow_uint_to_string((unsigned int)num, buffer, 10);
                                terminal_writestring(buffer);
                                chars_written += len;
                                break;
                            }
                            default:
                                terminal_putchar('%');
                                terminal_putchar('l');
                                terminal_putchar(format[i]);
                                chars_written += 3;
                                break;
                        }
                    }
                    break;
                }
                
                case '%': {
                    terminal_putchar('%');
                    chars_written++;
                    break;
                }
                
                default: {
                    /* Unsupported format specifier */
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    chars_written += 2;
                    break;
                }
            }
        }
    }
    
    va_end(args);
    terminal_writestring("\n");
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    return chars_written;
}

