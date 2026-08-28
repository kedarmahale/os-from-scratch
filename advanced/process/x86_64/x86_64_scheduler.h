#ifndef MEOWKERNEL_X86_64_SCHEDULER_H
#define MEOWKERNEL_X86_64_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include "../../hal/x86_64/x86_64_interrupts.h"

typedef void (*x86_64_task_entry_t)(void *argument);

typedef enum {
    SCHEDULER64_OK = 0,
    SCHEDULER64_ERR_NOT_INITIALIZED,
    SCHEDULER64_ERR_ALREADY_INITIALIZED,
    SCHEDULER64_ERR_INVALID_ENTRY,
    SCHEDULER64_ERR_INVALID_STACK,
    SCHEDULER64_ERR_OUT_OF_MEMORY,
    SCHEDULER64_ERR_TASK_LIMIT
} scheduler64_status_t;

typedef struct x86_64_scheduler_stats {
    uint64_t timer_ticks;
    uint64_t context_switches;
    uint32_t runnable_tasks;
    uint32_t current_pid;
} x86_64_scheduler_stats_t;

scheduler64_status_t x86_64_scheduler_init(void);
uint32_t x86_64_scheduler_create(const char *name,
                                 x86_64_task_entry_t entry,
                                 void *argument,
                                 size_t stack_size);
x86_64_interrupt_frame_t *x86_64_scheduler_timer_tick(
    x86_64_interrupt_frame_t *frame);
scheduler64_status_t x86_64_scheduler_get_stats(x86_64_scheduler_stats_t *stats);

#endif
