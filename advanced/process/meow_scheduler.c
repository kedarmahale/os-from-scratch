/* advanced/process/meow_scheduler.c - Task Scheduler Implementation
 *
 * Round-Robin scheduler with priority support
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_task.h"
#include "meow_scheduler.h"
#include "../../kernel/meow_util.h"

/* ================================================================
 * SCHEDULER STATE
 * ================================================================ */

static uint32_t scheduler_initialized = 0;
static uint32_t time_slice_counter = 0;
static uint32_t schedule_count = 0;

extern task_t* current_task;

/* ================================================================
 * SCHEDULER IMPLEMENTATION
 * ================================================================ */

/**
 * scheduler_init - Initialize the task scheduler
 */
meow_error_t scheduler_init(void)
{
    meow_log(MEOW_LOG_MEOW, "⏰ Initializing task scheduler...");
    
    /* Register timer callback for preemptive scheduling */
    meow_error_t result = HAL_TIMER_OP(register_callback, scheduler_tick);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to register scheduler timer callback");
        return result;
    }
    
    time_slice_counter = 0;
    schedule_count = 0;
    scheduler_initialized = 1;
    
    meow_log(MEOW_LOG_CHIRP, "😺 Task scheduler initialized - cats ready to multitask!");
    return MEOW_SUCCESS;
}

/**
 * scheduler_tick - Called by timer interrupt for preemptive scheduling
 */
void scheduler_tick(void)
{
    if (!scheduler_initialized) return;
    
    time_slice_counter++;
    
    /* Get current task */
    task_t* current = task_get_current();
    if (!current) return;
    
    /* Decrement time slice */
    if (current->time_slice > 0) {
        current->time_slice--;
    }
    
    /* If time slice expired, schedule next task */
    if (current->time_slice == 0) {
        schedule_next_task();
    }
}

/**
 * schedule_next_task - Select and switch to next task
 */
void schedule_next_task(void)
{
    task_t* current = task_get_current();
    task_t* next = select_next_task();
    
    if (!next) {
        /* No other tasks to run, stay with current */
        if (current) {
            current->time_slice = 10; /* Reset time slice */
        }
        return;
    }
    
    if (next == current) {
        /* Same task, just reset time slice */
        current->time_slice = 10;
        return;
    }
    
    /* Update statistics */
    schedule_count++;
    
    /* Set previous task to ready (if it was running) */
    if (current && current->state == TASK_STATE_RUNNING) {
        task_set_state(current->pid, TASK_STATE_READY);
    }
    
    /* Set next task to running */
    task_set_state(next->pid, TASK_STATE_RUNNING);
    next->time_slice = 10; /* 10ms time slice */
    next->last_scheduled = HAL_TIMER_OP(get_ticks);
    
    meow_log(MEOW_LOG_PURR, "🔄 Scheduling task '%s' (PID %u)", next->name, next->pid);
    
    /* Perform context switch */
    if (current) {
        task_switch_context(current, next);
    }
    
    /* Update current task pointer */
    /* Note: This would normally be done in assembly context switch */
    /* For now, we'll do it here */
    set_current_task(next);
}

/**
 * select_next_task - Select next task to run (Round-Robin with priorities)
 */
task_t* select_next_task(void)
{
    task_t* best_task = NULL;
    task_priority_t highest_priority = PRIORITY_IDLE;
    
    /* First pass: Find highest priority ready task */
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_t* task = task_get_table_entry(i);
        if (task && task->state == TASK_STATE_READY) {
            if (task->priority > highest_priority) {
                highest_priority = task->priority;
                best_task = task;
            }
        }
    }
    
    /* If no ready tasks found, look for idle task */
    if (!best_task) {
        for (uint32_t i = 0; i < MAX_TASKS; i++) {
            task_t* task = task_get_table_entry(i);
            if (task && task->state == TASK_STATE_READY && task->priority == PRIORITY_IDLE) {
                best_task = task;
                break;
            }
        }
    }
    
    /* Wake up sleeping tasks (simplified implementation) */
    wake_sleeping_tasks();
    
    return best_task;
}

/**
 * wake_sleeping_tasks - Wake up tasks that have finished sleeping
 */
void wake_sleeping_tasks(void)
{
    uint64_t current_ticks = HAL_TIMER_OP(get_ticks);
    
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_t* task = task_get_table_entry(i);
        if (task && task->state == TASK_STATE_BLOCKED) {
            /* Check if sleep time has elapsed (simplified) */
            if (task->last_scheduled > 0 && current_ticks >= task->last_scheduled) {
                meow_log(MEOW_LOG_PURR, "😺 Waking up task '%s' (PID %u)", task->name, task->pid);
                task_set_state(task->pid, TASK_STATE_READY);
                task->last_scheduled = 0;
            }
        }
    }
}

/**
 * scheduler_yield - Handle voluntary task yield
 */
void scheduler_yield(void)
{
    task_t* current = task_get_current();
    if (current) {
        /* Reset time slice to force rescheduling */
        current->time_slice = 0;
        schedule_next_task();
    }
}

/**
 * set_current_task - Set the current running task (internal use)
 */
void set_current_task(task_t* task)
{
    /* This would normally be done atomically */
    HAL_CPU_OP(disable_interrupts);
    
    /* Update global current task pointer */
    //extern task_t* current_task; /* From task.c */
    current_task = task;
    
    HAL_CPU_OP(enable_interrupts);
}

/**
 * scheduler_block_current - Block current task and reschedule
 */
void scheduler_block_current(void)
{
    task_t* current = task_get_current();
    if (current) {
        task_set_state(current->pid, TASK_STATE_BLOCKED);
        schedule_next_task();
    }
}

/**
 * scheduler_get_stats - Get scheduler statistics
 */
meow_error_t scheduler_get_stats(scheduler_stats_t* stats)
{
    MEOW_RETURN_IF_NULL(stats);
    
    stats->total_schedules = schedule_count;
    stats->timer_ticks = time_slice_counter;
    stats->current_pid = task_get_current() ? task_get_current()->pid : 0;
    
    return MEOW_SUCCESS;
}

