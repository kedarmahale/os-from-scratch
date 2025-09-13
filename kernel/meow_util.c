/* kernel/meow_util.c - MeowKernel Common Utilities */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "meow_util.h"
#include "limits.h"

/* Forward declarations for terminal functions */
extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);

/* Global debug level */
static meow_debug_level_t current_debug_level = MEOW_DEBUG_MEOW;

/* Uptime tracking */
static uint64_t meow_boot_time_ms = 0;

/* ================================
 * MEOW STRING UTILITIES
 * ================================ */

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
static void meow_reverse_string(char* str, int length) {
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

/* ================================
 *  MEOW PRINTF IMPLEMENTATION
 * ================================ */

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

/* Additional printf-like functions */
int meow_putchar(int c) {
    terminal_putchar((char)c);
    return c;
}

int meow_puts(const char* str) {
    if (str == NULL) {
        return 0;
    }
    
    terminal_writestring(str);
    terminal_putchar('\n');
    return meow_strlen(str) + 1;
}

/* ================================
 *  MEOW DEBUGGING UTILITIES
 * ================================ */

/* Set debug level */
void meow_set_debug_level(meow_debug_level_t level) {
    current_debug_level = level;
    const char* level_names[] = {"PURR", "MEOW", "HISS", "ROAR"};
    meow_printf(" Debug level set to: %s\n", level_names[level]);
}

/* Debug output with levels */
void meow_debug(meow_debug_level_t level, const char* format, ...) {
    if (level <= current_debug_level) {
        const char* prefixes[] = {" PURR: ", " MEOW: ", " HISS: ", " ROAR: "};
        
        terminal_writestring(prefixes[level]);
        
        va_list args;
        va_start(args, format);
        
        char buffer[64];
        for (int i = 0; format[i] != '\0'; i++) {
            if (format[i] != '%') {
                terminal_putchar(format[i]);
            } else {
                i++;
                switch (format[i]) {
                    case 's': {
                        char* str = va_arg(args, char*);
                        if (str) terminal_writestring(str);
                        break;
                    }
                    case 'd': {
                        int num = va_arg(args, int);
                        meow_int_to_string(num, buffer, 10);
                        terminal_writestring(buffer);
                        break;
                    }
                    case 'u': {
                        unsigned int num = va_arg(args, unsigned int);
                        meow_uint_to_string(num, buffer, 10);
                        terminal_writestring(buffer);
                        break;
                    }
                    case 'x': {
                        unsigned int num = va_arg(args, unsigned int);
                        meow_uint_to_string(num, buffer, 16);
                        terminal_writestring(buffer);
                        break;
                    }
                    default:
                        terminal_putchar('%');
                        terminal_putchar(format[i]);
                        break;
                }
            }
        }
        
        va_end(args);
    }
}

/* Print memory dump in hex format */
void meow_mem_dump(const void* ptr, size_t size) {
    const unsigned char* data = (const unsigned char*)ptr;
    
    meow_printf("🐾 Memory dump at %p (%u bytes):\n", ptr, (unsigned int)size);
    
    for (size_t i = 0; i < size; i += 16) {
        /* Print address */
        meow_printf("%p: ", (void*)((uintptr_t)data + i));
        
        /* Print hex values */
        for (size_t j = 0; j < 16 && (i + j) < size; j++) {
            meow_printf("%02x ", data[i + j]);
        }
        
        /* Print ASCII representation */
        meow_printf(" |");
        for (size_t j = 0; j < 16 && (i + j) < size; j++) {
            char c = data[i + j];
            meow_putchar((c >= 32 && c <= 126) ? c : '.');
        }
        meow_printf("|\n");
    }
}

/* Assert function for debugging */
void meow_assert(int condition, const char* message, const char* file, int line) {
    if (!condition) {
        meow_printf("\n MEOW ASSERTION FAILED: %s\n", message);
        meow_printf("   File: %s\n", file);
        meow_printf("   Line: %d\n", line);
        meow_printf("    System halted - no more purring.\n");
        
        /* Halt the system */
        while (1) {
            asm volatile("hlt");
        }
    }
}

/* Panic function */
void meow_panic(const char* message) {
    meow_printf("\nMEOW PANIC: %s\n", message);
    meow_printf(" System catastrophe! No more purring allowed.\n");
    meow_printf(" MeowKernel has stopped responding to treats.\n");
    
    /* Disable interrupts and halt */
    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

/* ================================
 *  MEOW MATH UTILITIES
 * ================================ */

/* Absolute value */
int meow_abs(int x) {
    return (x < 0) ? -x : x;
}

/* Minimum of two values */
int meow_min(int a, int b) {
    return (a < b) ? a : b;
}

/* Maximum of two values */
int meow_max(int a, int b) {
    return (a > b) ? a : b;
}

/* Power function (integer) */
int meow_pow(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        if (exp & 1) {
            result *= base;
        }
        base *= base;
        exp >>= 1;
    }
    return result;
}

/* ================================
 *  MEOW SYSTEM UTILITIES
 * ================================ */

/* Spin wait for microseconds */
void meow_spin_wait(unsigned int microseconds) {
    /* Simple busy wait - not accurate but functional */
    volatile unsigned int count = microseconds * 1000; /* Rough approximation */
    while (count--) {
        asm volatile("nop");
    }
}

/* Get system uptime in milliseconds */
uint64_t meow_get_uptime_ms(void) {
    /* This would need timer integration */
    return meow_boot_time_ms;
}

/* Display system information */
void meow_system_info(void) {
    meow_printf("\n ===== MeowKernel System Information =====\n");
    meow_printf(" Kernel: MeowKernel v1.0 (Purr-fect Edition)\n");
    meow_printf(" Architecture: Detected automatically\n");
    meow_printf(" Uptime: %llu ms\n", meow_get_uptime_ms());
    meow_printf(" Debug Level: ");
    
    const char* level_names[] = {"PURR (Quiet)", "MEOW (Normal)", "HISS (Verbose)", "ROAR (Maximum)"};
    meow_printf("%s\n", level_names[current_debug_level]);
    
    meow_printf(" Status: Purring smoothly!\n");
    meow_printf(" ==========================================\n\n");
}
 
