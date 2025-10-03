/* advanced/syscalls/syscall_table.c - System Call Implementation
 *
 * Extends existing interrupt system for user-kernel interface
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_syscall_table.h"
#include "../../kernel/meow_util.h"
#include "../process/meow_task.h"
#include "../drivers/keyboard/meow_keyboard.h"

/* ================================================================
 * SYSTEM CALL TABLE AND STATE
 * ================================================================ */

static syscall_info_t syscall_table[MAX_SYSCALLS];
static syscall_stats_t syscall_statistics = {0};
static uint8_t syscall_initialized = 0;

/* ================================================================
 * SYSTEM CALL INITIALIZATION
 * ================================================================ */

meow_error_t syscall_init(void)
{
    meow_log(MEOW_LOG_MEOW, "📞 Initializing system call interface...");
    
    /* Clear system call table */
    for (uint32_t i = 0; i < MAX_SYSCALLS; i++) {
        syscall_table[i].handler = NULL;
        syscall_table[i].name = NULL;
        syscall_table[i].arg_count = 0;
        syscall_table[i].implemented = 0;
    }
    
    /* Clear statistics */
    syscall_statistics.total_calls = 0;
    syscall_statistics.successful_calls = 0;
    syscall_statistics.failed_calls = 0;
    syscall_statistics.invalid_calls = 0;
    
    for (uint32_t i = 0; i < MAX_SYSCALLS; i++) {
        syscall_statistics.call_counts[i] = 0;
    }
    
    /* Register interrupt handler for syscalls (int 0x80) */
    meow_error_t result = HAL_INTERRUPT_OP(register_handler, 0x80, syscall_interrupt_handler);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to register syscall interrupt handler");
        return result;
    }
    
    /* Register basic system calls */
    syscall_register(SYS_EXIT, sys_exit, "exit", 1);
    syscall_register(SYS_WRITE, sys_write, "write", 3);
    syscall_register(SYS_READ, sys_read, "read", 3);
    syscall_register(SYS_GETPID, sys_getpid, "getpid", 0);
    syscall_register(SYS_YIELD, sys_yield, "yield", 0);
    syscall_register(SYS_SLEEP, sys_sleep, "sleep", 1);
    syscall_register(SYS_TIME, sys_time, "time", 1);
    syscall_register(SYS_BRK, sys_brk, "brk", 1);
    syscall_register(SYS_MEOW_LOG, sys_meow_log, "meow_log", 2);
    syscall_register(SYS_GET_STATS, sys_get_stats, "get_stats", 2);
    
    /* Cat-themed system calls */
    syscall_register(SYS_PURR, sys_purr, "purr", 2);
    syscall_register(SYS_HISS, sys_hiss, "hiss", 2);
    syscall_register(SYS_CHIRP, sys_chirp, "chirp", 2);
    
    syscall_initialized = 1;
    
    meow_log(MEOW_LOG_CHIRP, "😺 System call interface ready - cats can make requests!");
    return MEOW_SUCCESS;
}

meow_error_t syscall_register(uint32_t syscall_num, 
                              syscall_handler_t handler,
                              const char* name,
                              uint8_t arg_count)
{
    if (syscall_num >= MAX_SYSCALLS) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    if (!handler || !name) {
        return MEOW_ERROR_NULL_POINTER;
    }
    
    syscall_table[syscall_num].handler = handler;
    syscall_table[syscall_num].name = name;
    syscall_table[syscall_num].arg_count = arg_count;
    syscall_table[syscall_num].implemented = 1;
    
    meow_log(MEOW_LOG_PURR, "📞 Registered syscall %u: %s (%u args)", 
             syscall_num, name, arg_count);
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * SYSTEM CALL INTERRUPT HANDLER
 * ================================================================ */
#if 0
void syscall_interrupt_handler(uint8_t irq)
{
    /* Get system call parameters from CPU registers */
    uint32_t syscall_num, arg1, arg2, arg3, arg4;
    
    __asm__ volatile (
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esi, %4\n"
        : "=m"(syscall_num), "=m"(arg1), "=m"(arg2), "=m"(arg3), "=m"(arg4)
    );
    
    /* Call dispatcher */
    meow_error_t result = syscall_dispatcher(syscall_num, arg1, arg2, arg3, arg4);
    
    /* Set return value in EAX */
    __asm__ volatile ("mov %0, %%eax" : : "r"(result));
}
#endif

meow_error_t syscall_dispatcher(uint32_t syscall_num, 
                               uint32_t arg1, uint32_t arg2,
                               uint32_t arg3, uint32_t arg4)
{
    if (!syscall_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Update statistics */
    syscall_statistics.total_calls++;
    
    /* Validate system call number */
    if (syscall_num >= MAX_SYSCALLS) {
        syscall_statistics.invalid_calls++;
        meow_log(MEOW_LOG_HISS, "😾 Invalid syscall number: %u", syscall_num);
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if system call is implemented */
    if (!syscall_table[syscall_num].implemented || !syscall_table[syscall_num].handler) {
        syscall_statistics.invalid_calls++;
        meow_log(MEOW_LOG_HISS, "😾 Unimplemented syscall: %u", syscall_num);
        return MEOW_ERROR_NOT_SUPPORTED;
    }
    
    /* Update call count for this syscall */
    syscall_statistics.call_counts[syscall_num]++;
    
    /* Log syscall (debug level) */
    meow_log(MEOW_LOG_PURR, "📞 Syscall %u (%s) called with args: %u, %u, %u, %u", 
             syscall_num, syscall_table[syscall_num].name, arg1, arg2, arg3, arg4);
    
    /* Call the handler */
    meow_error_t result = syscall_table[syscall_num].handler(arg1, arg2, arg3, arg4);
    
    /* Update statistics */
    if (result == MEOW_SUCCESS) {
        syscall_statistics.successful_calls++;
    } else {
        syscall_statistics.failed_calls++;
        meow_log(MEOW_LOG_HISS, "😾 Syscall %u failed with error: %s", 
                 syscall_num, meow_error_to_string(result));
    }
    
    return result;
}

meow_error_t syscall_get_stats(syscall_stats_t* stats)
{
    MEOW_RETURN_IF_NULL(stats);
    
    *stats = syscall_statistics;
    return MEOW_SUCCESS;
}

/* ================================================================
 * BASIC SYSTEM CALL IMPLEMENTATIONS
 * ================================================================ */

meow_error_t sys_exit(uint32_t exit_code, uint32_t unused1, 
                      uint32_t unused2, uint32_t unused3)
{
    task_t* current = task_get_current();
    if (!current) {
        return MEOW_ERROR_INVALID_STATE;
    }
    
    meow_log(MEOW_LOG_PURR, "😴 Task '%s' (PID %u) exiting with code %u", 
             current->name, current->pid, exit_code);
    
    return task_terminate(current->pid, (int)exit_code);
}

meow_error_t sys_getpid(uint32_t unused1, uint32_t unused2, 
                        uint32_t unused3, uint32_t unused4)
{
    task_t* current = task_get_current();
    if (!current) {
        return 0; /* Return 0 if no current task */
    }
    
    return current->pid;
}

meow_error_t sys_yield(uint32_t unused1, uint32_t unused2, 
                       uint32_t unused3, uint32_t unused4)
{
    task_yield();
    return MEOW_SUCCESS;
}

meow_error_t sys_write(uint32_t fd, uint32_t buffer, 
                       uint32_t count, uint32_t unused)
{
    /* Validate file descriptor */
    if (fd > 2) { /* Only support stdout/stderr for now */
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* Validate buffer pointer */
    if (!syscall_validate_pointer((void*)buffer, count)) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* For stdout/stderr, write to VGA display */
    char* str = (char*)buffer;
    for (uint32_t i = 0; i < count; i++) {
        if (str[i] == '\0') break;
        meow_putc(str[i]);
    }
    
    return count; /* Return number of bytes written */
}

meow_error_t sys_read(uint32_t fd, uint32_t buffer, 
                      uint32_t count, uint32_t unused)
{
    /* Only support stdin for now */
    if (fd != 0) {
        return MEOW_ERROR_NOT_SUPPORTED;
    }
    
    /* Validate buffer pointer */
    if (!syscall_validate_pointer((void*)buffer, count)) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* For stdin, read from keyboard */
    char* buf = (char*)buffer;
    uint32_t bytes_read = 0;
    
    while (bytes_read < count - 1) {
        char ch = keyboard_getchar(); /* This will block */
        buf[bytes_read++] = ch;
        
        if (ch == '\n') break;
    }
    
    buf[bytes_read] = '\0';
    return bytes_read;
}

meow_error_t sys_sleep(uint32_t milliseconds, uint32_t unused1, 
                       uint32_t unused2, uint32_t unused3)
{
    task_sleep(milliseconds);
    return MEOW_SUCCESS;
}

meow_error_t sys_time(uint32_t time_ptr, uint32_t unused1, 
                      uint32_t unused2, uint32_t unused3)
{
    uint64_t ticks = HAL_TIMER_OP(get_ticks);
    uint64_t seconds = ticks / 100; /* Assuming 100Hz timer */
    
    if (time_ptr != 0) {
        /* Copy time to user space */
        if (!syscall_validate_pointer((void*)time_ptr, sizeof(uint64_t))) {
            return MEOW_ERROR_INVALID_PARAMETER;
        }
        
        *(uint64_t*)time_ptr = seconds;
    }
    
    return (meow_error_t)seconds;
}

meow_error_t sys_brk(uint32_t addr, uint32_t unused1, 
                     uint32_t unused2, uint32_t unused3)
{
    /* Simple implementation - just return current heap end */
    /* In a real implementation, this would manage process heap */
    return (meow_error_t)0x400000; /* Fake heap end */
}

/* ================================================================
 * CAT-THEMED SYSTEM CALLS
 * ================================================================ */

meow_error_t sys_purr(uint32_t duration, uint32_t volume, 
                      uint32_t unused1, uint32_t unused2)
{
    meow_log(MEOW_LOG_PURR, "😻 *purrrrr* (duration: %ums, volume: %u)", duration, volume);
    
    /* Simulate purring by sleeping */
    for (uint32_t i = 0; i < duration / 100; i++) {
        meow_log(MEOW_LOG_PURR, "😸 *purr*");
        task_sleep(100);
    }
    
    return MEOW_SUCCESS;
}

meow_error_t sys_hiss(uint32_t warning_level, uint32_t message_ptr, 
                      uint32_t unused1, uint32_t unused2)
{
    const char* message = "Warning!";
    
    if (message_ptr != 0) {
        if (syscall_validate_string((char*)message_ptr, 256) > 0) {
            message = (char*)message_ptr;
        }
    }
    
    switch (warning_level) {
        case 1:
            meow_log(MEOW_LOG_HISS, "😾 *hiss* %s", message);
            break;
        case 2:
            meow_log(MEOW_LOG_YOWL, "🙀 *HISSSSS* %s", message);
            break;
        default:
            meow_log(MEOW_LOG_HISS, "😼 *soft hiss* %s", message);
            break;
    }
    
    return MEOW_SUCCESS;
}

meow_error_t sys_chirp(uint32_t happiness_level, uint32_t message_ptr, 
                       uint32_t unused1, uint32_t unused2)
{
    const char* message = "Happy cat sounds!";
    
    if (message_ptr != 0) {
        if (syscall_validate_string((char*)message_ptr, 256) > 0) {
            message = (char*)message_ptr;
        }
    }
    
    for (uint32_t i = 0; i < happiness_level && i < 5; i++) {
        meow_log(MEOW_LOG_CHIRP, "😺 *chirp chirp* %s", message);
        task_sleep(50);
    }
    
    return MEOW_SUCCESS;
}

meow_error_t sys_meow_log(uint32_t level, uint32_t message_ptr, 
                          uint32_t unused1, uint32_t unused2)
{
    if (!syscall_validate_pointer((void*)message_ptr, 1)) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    if (syscall_validate_string((char*)message_ptr, 512) <= 0) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    if (level > MEOW_LOG_SCREECH) {
        level = MEOW_LOG_MEOW;
    }
    
    meow_log((meow_log_level_t)level, "%s", (char*)message_ptr);
    return MEOW_SUCCESS;
}

meow_error_t sys_get_stats(uint32_t stats_ptr, uint32_t stats_type, 
                           uint32_t unused1, uint32_t unused2)
{
    if (!syscall_validate_pointer((void*)stats_ptr, sizeof(uint64_t))) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    switch (stats_type) {
        case 0: /* Syscall stats */
            if (!syscall_validate_pointer((void*)stats_ptr, sizeof(syscall_stats_t))) {
                return MEOW_ERROR_INVALID_PARAMETER;
            }
            return syscall_copy_to_user((void*)stats_ptr, &syscall_statistics, 
                                        sizeof(syscall_stats_t));
            
        case 1: /* Task stats */
            if (!syscall_validate_pointer((void*)stats_ptr, sizeof(task_statistics_t))) {
                return MEOW_ERROR_INVALID_PARAMETER;
            }
            task_statistics_t task_stats;
            task_get_statistics(&task_stats);
            return syscall_copy_to_user((void*)stats_ptr, &task_stats, 
                                        sizeof(task_statistics_t));
            
        default:
            return MEOW_ERROR_INVALID_PARAMETER;
    }
}

/* ================================================================
 * VALIDATION HELPER FUNCTIONS
 * ================================================================ */

int syscall_validate_pointer(const void* ptr, size_t size)
{
    /* Basic validation - check if pointer is in valid range */
    uintptr_t addr = (uintptr_t)ptr;
    
    /* Reject NULL pointers */
    if (addr == 0) return 0;
    
    /* Reject kernel space addresses (simplified check) */
    if (addr < 0x1000 || addr >= 0x80000000) return 0;
    
    /* Check if range is valid */
    if (addr + size < addr) return 0; /* Overflow check */
    
    return 1; /* Valid */
}

int syscall_validate_string(const char* str, size_t max_len)
{
    if (!syscall_validate_pointer(str, 1)) return -1;
    
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    
    if (len >= max_len) return -1; /* String too long or not null-terminated */
    
    return (int)len;
}

meow_error_t syscall_copy_from_user(void* dest, const void* src, size_t size)
{
    if (!syscall_validate_pointer(src, size)) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    meow_memcpy(dest, src, size);
    return MEOW_SUCCESS;
}

meow_error_t syscall_copy_to_user(void* dest, const void* src, size_t size)
{
    if (!syscall_validate_pointer(dest, size)) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    meow_memcpy(dest, src, size);
    return MEOW_SUCCESS;
}