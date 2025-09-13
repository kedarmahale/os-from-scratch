/* kernel/meow_util.h - MeowKernel Utilities Header */

#ifndef MEOW_UTIL_H
#define MEOW_UTIL_H

#include <stddef.h>
#include <stdarg.h>

/* ================================
 *  MEOW STRING UTILITIES
 * ================================ */
size_t meow_strlen(const char* str);
int meow_strcmp(const char* str1, const char* str2);
int meow_strncmp(const char* str1, const char* str2, size_t n);
char* meow_strcpy(char* dest, const char* src);
char* meow_strncpy(char* dest, const char* src, size_t n);
char* meow_strcat(char* dest, const char* src);
char* meow_strchr(const char* str, int c);

/* ================================
 *  MEOW MEMORY UTILITIES
 * ================================ */
void* meow_memset(void* ptr, int value, size_t num);
void* meow_memcpy(void* dest, const void* src, size_t num);
void* meow_memmove(void* dest, const void* src, size_t num);
int meow_memcmp(const void* ptr1, const void* ptr2, size_t num);

/* ================================
 *  MEOW NUMBER CONVERSION
 * ================================ */
int meow_int_to_string(int num, char* str, int base);
int meow_uint_to_string(unsigned int num, char* str, int base);
int meow_longlong_to_string(long long num, char* str, int base);
int meow_atoi(const char* str);

/* ================================
 *  MEOW PRINTF UTILITIES
 * ================================ */
int meow_printf(const char* format, ...);
int meow_putchar(int c);
int meow_puts(const char* str);

/* ================================
 * MEOW DEBUGGING UTILITIES
 * ================================ */
void meow_mem_dump(const void* ptr, size_t size);
void meow_assert(int condition, const char* message, const char* file, int line);
void meow_panic(const char* message);

/* Debug macros with meow theme */
#define MEOW_ASSERT(condition) meow_assert((condition), #condition, __FILE__, __LINE__)
#define MEOW_PANIC(message) meow_panic(message)

/* Cat-themed debug levels */
typedef enum {
    MEOW_DEBUG_PURR = 0,    /* Quiet/minimal debug */
    MEOW_DEBUG_MEOW = 1,    /* Normal debug */
    MEOW_DEBUG_HISS = 2,    /* Verbose debug */
    MEOW_DEBUG_ROAR = 3     /* Maximum debug */
} meow_debug_level_t;

void meow_set_debug_level(meow_debug_level_t level);
void meow_debug(meow_debug_level_t level, const char* format, ...);

/* ================================
 *  MEOW MATH UTILITIES
 * ================================ */
int meow_abs(int x);
int meow_min(int a, int b);
int meow_max(int a, int b);
int meow_pow(int base, int exp);

/* ================================
 *  MEOW SYSTEM UTILITIES
 * ================================ */
void meow_spin_wait(unsigned int microseconds);
uint64_t meow_get_uptime_ms(void);
void meow_system_info(void);

#endif /* MEOW_UTIL_H */

