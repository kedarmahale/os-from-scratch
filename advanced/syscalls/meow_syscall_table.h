/* advanced/syscalls/meow_syscall_table.h - MeowKernel System Call Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_SYSCALL_TABLE_H
#define MEOW_SYSCALL_TABLE_H

#include <stdint.h>
#include "stddef.h" // for size_t
#include "../kernel/meow_error_definitions.h"

/* ================================================================
 * SYSTEM CALL NUMBERS
 * ================================================================ */

/* Basic system calls */
#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2
#define SYS_OPEN        3
#define SYS_CLOSE       4

/* Process management */
#define SYS_FORK        5
#define SYS_GETPID      6
#define SYS_GETPPID     7
#define SYS_WAIT        8
#define SYS_EXEC        9

/* Memory management */
#define SYS_BRK         10
#define SYS_MMAP        11
#define SYS_MUNMAP      12

/* Time and scheduling */
#define SYS_SLEEP       13
#define SYS_YIELD       14
#define SYS_TIME        15

/* Debugging and info */
#define SYS_MEOW_LOG    16
#define SYS_GET_STATS   17

/* Cat-themed system calls (MeowKernel specific) */
#define SYS_PURR        100  /* Make the kernel purr */
#define SYS_HISS        101  /* System warning */
#define SYS_CHIRP       102  /* Happy system message */

#define MAX_SYSCALLS    128

/* ================================================================
 * SYSTEM CALL HANDLER TYPES
 * ================================================================ */

/* System call handler function type */
typedef meow_error_t (*syscall_handler_t)(uint32_t arg1, uint32_t arg2, 
                                          uint32_t arg3, uint32_t arg4);

/* System call information structure */
typedef struct syscall_info {
    syscall_handler_t handler;
    const char* name;
    uint8_t arg_count;
    uint8_t implemented;
} syscall_info_t;

/* System call statistics */
typedef struct syscall_stats {
    uint64_t total_calls;
    uint64_t successful_calls;
    uint64_t failed_calls;
    uint64_t invalid_calls;
    uint32_t call_counts[MAX_SYSCALLS];
} syscall_stats_t;

/* ================================================================
 * SYSTEM CALL INTERFACE FUNCTIONS
 * ================================================================ */

/**
 * syscall_init - Initialize system call interface
 * 
 * Registers interrupt 0x80 handler using existing HAL
 */
meow_error_t syscall_init(void);

/**
 * syscall_register - Register a system call handler
 * @syscall_num: System call number
 * @handler: Handler function
 * @name: Descriptive name
 * @arg_count: Number of arguments
 */
meow_error_t syscall_register(uint32_t syscall_num, 
                              syscall_handler_t handler,
                              const char* name,
                              uint8_t arg_count);

/**
 * syscall_dispatcher - Main system call dispatcher
 * @syscall_num: System call number (from EAX)
 * @arg1: First argument (from EBX)
 * @arg2: Second argument (from ECX)
 * @arg3: Third argument (from EDX)
 * @arg4: Fourth argument (from ESI)
 * 
 * Called from assembly interrupt handler
 */
meow_error_t syscall_dispatcher(uint32_t syscall_num, 
                               uint32_t arg1, uint32_t arg2,
                               uint32_t arg3, uint32_t arg4);

/**
 * syscall_get_stats - Get system call statistics
 */
meow_error_t syscall_get_stats(syscall_stats_t* stats);

/* ================================================================
 * BASIC SYSTEM CALL IMPLEMENTATIONS
 * ================================================================ */

/* Process control */
meow_error_t sys_exit(uint32_t exit_code, uint32_t unused1, 
                      uint32_t unused2, uint32_t unused3);
meow_error_t sys_getpid(uint32_t unused1, uint32_t unused2, 
                        uint32_t unused3, uint32_t unused4);
meow_error_t sys_yield(uint32_t unused1, uint32_t unused2, 
                       uint32_t unused3, uint32_t unused4);

/* I/O operations (basic implementations) */
meow_error_t sys_write(uint32_t fd, uint32_t buffer, 
                       uint32_t count, uint32_t unused);
meow_error_t sys_read(uint32_t fd, uint32_t buffer, 
                      uint32_t count, uint32_t unused);

/* Time operations */
meow_error_t sys_sleep(uint32_t milliseconds, uint32_t unused1, 
                       uint32_t unused2, uint32_t unused3);
meow_error_t sys_time(uint32_t time_ptr, uint32_t unused1, 
                      uint32_t unused2, uint32_t unused3);

/* Memory operations */
meow_error_t sys_brk(uint32_t addr, uint32_t unused1, 
                     uint32_t unused2, uint32_t unused3);

/* Cat-themed system calls */
meow_error_t sys_purr(uint32_t duration, uint32_t volume, 
                      uint32_t unused1, uint32_t unused2);
meow_error_t sys_hiss(uint32_t warning_level, uint32_t message_ptr, 
                      uint32_t unused1, uint32_t unused2);
meow_error_t sys_chirp(uint32_t happiness_level, uint32_t message_ptr, 
                       uint32_t unused1, uint32_t unused2);

/* Debug and info */
meow_error_t sys_meow_log(uint32_t level, uint32_t message_ptr, 
                          uint32_t unused1, uint32_t unused2);
meow_error_t sys_get_stats(uint32_t stats_ptr, uint32_t stats_type, 
                           uint32_t unused1, uint32_t unused2);

/* ================================================================
 * USER-SPACE SYSTEM CALL WRAPPERS
 * ================================================================ */

/* These provide convenient C interface for user programs */

static inline void exit(int status) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}

static inline uint32_t getpid(void) {
    uint32_t pid;
    __asm__ volatile ("int $0x80" : "=a"(pid) : "a"(SYS_GETPID));
    return pid;
}

static inline void yield(void) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_YIELD));
}

static inline int write(int fd, const void* buf, size_t count) {
    int result;
    __asm__ volatile ("int $0x80" 
                     : "=a"(result) 
                     : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count));
    return result;
}

static inline int read(int fd, void* buf, size_t count) {
    int result;
    __asm__ volatile ("int $0x80" 
                     : "=a"(result) 
                     : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count));
    return result;
}

static inline void sleep(unsigned int seconds) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_SLEEP), "b"(seconds * 1000));
}

/* Cat-themed user functions */
static inline void purr(int duration) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_PURR), "b"(duration), "c"(50));
}

static inline void hiss(const char* warning) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_HISS), "b"(1), "c"(warning));
}

static inline void chirp(const char* message) {
    __asm__ volatile ("int $0x80" : : "a"(SYS_CHIRP), "b"(5), "c"(message));
}

/* ================================================================
 * SYSTEM CALL VALIDATION HELPERS
 * ================================================================ */

/**
 * syscall_validate_pointer - Validate user-space pointer
 * @ptr: Pointer to validate
 * @size: Size of memory region
 * 
 * @return 1 if valid, 0 if invalid
 */
int syscall_validate_pointer(const void* ptr, size_t size);

/**
 * syscall_validate_string - Validate user-space string
 * @str: String to validate
 * @max_len: Maximum allowed length
 * 
 * @return Length if valid, -1 if invalid
 */
int syscall_validate_string(const char* str, size_t max_len);

/**
 * syscall_copy_from_user - Safely copy data from user space
 * @dest: Kernel destination buffer
 * @src: User source pointer
 * @size: Number of bytes to copy
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t syscall_copy_from_user(void* dest, const void* src, size_t size);

/**
 * syscall_copy_to_user - Safely copy data to user space
 * @dest: User destination pointer
 * @src: Kernel source buffer
 * @size: Number of bytes to copy
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t syscall_copy_to_user(void* dest, const void* src, size_t size);

void syscall_interrupt_handler(uint8_t irq);

#endif /* MEOW_SYSCALL_TABLE_H */