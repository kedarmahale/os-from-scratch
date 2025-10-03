#ifndef MEOW_SCHEDULER_H
#define MEOW_SCHEDULER_H

#include "meow_task.h" // for task_t

// Declare the scheduler functions
meow_error_t scheduler_init(void);
void scheduler_tick(void);
void schedule_next_task(void);
task_t* select_next_task(void);
void set_current_task(task_t* task);
void wake_sleeping_tasks(void);

/* ================================================================
 * SCHEDULER STATISTICS TYPE
 * ================================================================ */

typedef struct {
    // Define relevant scheduler stats fields
    uint64_t timer_ticks;
    uint64_t total_schedules;
    uint64_t slept_tasks;
    uint64_t current_pid;
    // add more as needed
} scheduler_stats_t;

meow_error_t scheduler_get_stats(scheduler_stats_t* stats);

#endif // MEOW_SCHEDULER_H
