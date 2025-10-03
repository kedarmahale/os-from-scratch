/* kernel/meow_shell.c - Interactive Shell for MeowKernel
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_util.h"
#include "../advanced/drivers/keyboard/meow_keyboard.h"
#include "../advanced/process/meow_task.h"
#include "../advanced/syscalls/meow_syscall_table.h"
#include "../advanced/process/meow_task.h"

/* ================================================================
 * SHELL COMMAND PROCESSING
 * ================================================================ */

static void shell_cmd_help(void)
{
    meow_log(MEOW_LOG_CHIRP, "🐾 Available commands:");
    meow_log(MEOW_LOG_PURR, "  help     - Show this help");
    meow_log(MEOW_LOG_PURR, "  ps       - List processes");
    meow_log(MEOW_LOG_PURR, "  time     - Show system time");
    meow_log(MEOW_LOG_PURR, "  stats    - Show system statistics");
    meow_log(MEOW_LOG_PURR, "  meow     - Make the kernel meow!");
    meow_log(MEOW_LOG_PURR, "  purr     - Make the kernel purr!");
    meow_log(MEOW_LOG_PURR, "  test     - Run system tests");
    meow_log(MEOW_LOG_PURR, "  clear    - Clear screen");
    meow_log(MEOW_LOG_PURR, "  exit     - Exit shell");
}

static void shell_cmd_ps(void)
{
    task_dump_info(0);  /* Show all tasks */
}

static void shell_cmd_time(void)
{
    uint64_t ticks = HAL_TIMER_OP(get_ticks);
    uint64_t ms = HAL_TIMER_OP(get_milliseconds);
    meow_log(MEOW_LOG_CHIRP, "⏰ System time: %llu ticks (%llu ms)", ticks, ms);
}

static void shell_cmd_stats(void)
{
    /* Show keyboard statistics */
    keyboard_stats_t kb_stats;
    if (keyboard_get_stats(&kb_stats) == MEOW_SUCCESS) {
        meow_log(MEOW_LOG_CHIRP, "⌨️ Keyboard Statistics:");
        meow_log(MEOW_LOG_PURR, "  Keys pressed: %u", kb_stats.keys_pressed);
        meow_log(MEOW_LOG_PURR, "  Keys released: %u", kb_stats.keys_released);
        meow_log(MEOW_LOG_PURR, "  Buffer overflows: %u", kb_stats.buffer_overflows);
    }
    
    /* Show task statistics */
    task_statistics_t task_stats;
    if (task_get_statistics(&task_stats) == MEOW_SUCCESS) {
        meow_log(MEOW_LOG_CHIRP, "🔄 Task Statistics:");
        meow_log(MEOW_LOG_PURR, "  Total tasks: %u", task_stats.total_tasks);
        meow_log(MEOW_LOG_PURR, "  Running tasks: %u", task_stats.running_tasks);
        meow_log(MEOW_LOG_PURR, "  Context switches: %u", task_stats.context_switches);
    }
    
    /* Show system call statistics */
    syscall_stats_t sc_stats;
    if (syscall_get_stats(&sc_stats) == MEOW_SUCCESS) {
        meow_log(MEOW_LOG_CHIRP, "📞 System Call Statistics:");
        meow_log(MEOW_LOG_PURR, "  Total calls: %llu", sc_stats.total_calls);
        meow_log(MEOW_LOG_PURR, "  Successful calls: %llu", sc_stats.successful_calls);
        meow_log(MEOW_LOG_PURR, "  Failed calls: %llu", sc_stats.failed_calls);
    }
}

static void shell_cmd_meow(void)
{
    meow_log(MEOW_LOG_MEOW, "🐱 MEOOOOOW! *purr purr purr* 😸");
    meow_log(MEOW_LOG_CHIRP, "😺 The kernel is very happy you asked!");
}

static void shell_cmd_purr(void)
{
    meow_log(MEOW_LOG_PURR, "😻 *purrrrrrr* *purrrrrrr* *purrrrrrr*");
    meow_log(MEOW_LOG_PURR, "😴 The kernel is content and sleepy...");
}

static void shell_cmd_test(void)
{
    meow_log(MEOW_LOG_MEOW, "🧪 Running system tests...");
    
    /* Test system calls */
    meow_log(MEOW_LOG_CHIRP, "📞 Testing system calls...");
    uint32_t pid = getpid();
    meow_log(MEOW_LOG_PURR, "  Current PID: %u", pid);
    
    /* Test cat-themed system calls */
    purr(100);  /* Purr for 100ms */
    chirp("System test in progress!");
    
    meow_log(MEOW_LOG_CHIRP, "✅ System tests completed!");
}

static void shell_cmd_clear(void)
{
    clear_screen();
    meow_log(MEOW_LOG_CHIRP, "🧹 Screen cleared - fresh start for cats!");
}

/* ================================================================
 * MAIN SHELL FUNCTION
 * ================================================================ */

void meow_shell_run(void)
{
    char input_buffer[256];
    int running = 1;
    
    meow_log(MEOW_LOG_CHIRP, "🐾 MeowKernel Interactive Shell Started!");
    meow_log(MEOW_LOG_MEOW, "Type 'help' for commands, 'exit' to quit");
    
    while (running) {
        meow_printf("🐱 meow> ");
        
        /* Get user input using keyboard driver */
        int len = keyboard_gets(input_buffer, sizeof(input_buffer));
        
        if (len > 0) {
            /* Process commands */
            if (meow_strcmp(input_buffer, "help") == 0) {
                shell_cmd_help();
            } else if (meow_strcmp(input_buffer, "ps") == 0) {
                shell_cmd_ps();
            } else if (meow_strcmp(input_buffer, "time") == 0) {
                shell_cmd_time();
            } else if (meow_strcmp(input_buffer, "stats") == 0) {
                shell_cmd_stats();
            } else if (meow_strcmp(input_buffer, "meow") == 0) {
                shell_cmd_meow();
            } else if (meow_strcmp(input_buffer, "purr") == 0) {
                shell_cmd_purr();
            } else if (meow_strcmp(input_buffer, "test") == 0) {
                shell_cmd_test();
            } else if (meow_strcmp(input_buffer, "clear") == 0) {
                shell_cmd_clear();
            } else if (meow_strcmp(input_buffer, "exit") == 0) {
                meow_log(MEOW_LOG_PURR, "😴 Shell exiting - goodbye from the cats!");
                running = 0;
            } else if (meow_strlen(input_buffer) > 0) {
                meow_log(MEOW_LOG_HISS, "😾 Unknown command: '%s' (try 'help')", input_buffer);
            }
        }
        
        /* Small delay to prevent overwhelming the system */
        task_sleep(50);
    }
}

/* ================================================================
 * DEMO TASK FUNCTIONS
 * ================================================================ */



