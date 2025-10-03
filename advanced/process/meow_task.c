/* advanced/process/meow_task.c - Task Management Implementation
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_task.h"
#include "../../kernel/meow_util.h"
#include "../../kernel/meow_error_definitions.h"
#include "../mm/meow_heap_allocator.h"

/* ================================================================
 * GLOBAL TASK STATE
 * ================================================================ */

/* Task table - static allocation for simplicity */
static task_t task_table[MAX_TASKS];
static uint32_t next_pid = 1;
//static task_t* current_task = NULL;
static task_statistics_t task_stats = {0};

/* Task queues for scheduling */
static task_t* ready_queue_head = NULL;
static task_t* blocked_queue_head = NULL;

task_t* current_task = NULL;
/* ================================================================
 * INTERNAL HELPER FUNCTIONS
 * ================================================================ */

static task_t* find_free_task_slot(void)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].state == TASK_STATE_UNUSED) {
            return &task_table[i];
        }
    }
    return NULL;
}

static void add_to_ready_queue(task_t* task)
{
    task->next = ready_queue_head;
    task->prev = NULL;
    if (ready_queue_head) {
        ready_queue_head->prev = task;
    }
    ready_queue_head = task;
}

static void remove_from_ready_queue(task_t* task)
{
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        ready_queue_head = task->next;
    }
    
    if (task->next) {
        task->next->prev = task->prev;
    }
    
    task->next = task->prev = NULL;
}

static void add_to_blocked_queue(task_t* task)
{
    task->next = blocked_queue_head;
    task->prev = NULL;
    if (blocked_queue_head) {
        blocked_queue_head->prev = task;
    }
    blocked_queue_head = task;
}

static void remove_from_blocked_queue(task_t* task)
{
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        blocked_queue_head = task->next;
    }
    
    if (task->next) {
        task->next->prev = task->prev;
    }
    
    task->next = task->prev = NULL;
}

/* ================================================================
 * TASK MANAGEMENT IMPLEMENTATION
 * ================================================================ */

meow_error_t task_system_init(void)
{
    meow_log(MEOW_LOG_MEOW, "🔄 Initializing task management system...");
    
    /* Clear task table */
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_table[i].state = TASK_STATE_UNUSED;
        task_table[i].pid = 0;
        task_table[i].next = NULL;
        task_table[i].prev = NULL;
    }
    
    /* Clear statistics */
    task_stats.total_tasks = 0;
    task_stats.running_tasks = 0;
    task_stats.ready_tasks = 0;
    task_stats.blocked_tasks = 0;
    task_stats.terminated_tasks = 0;
    task_stats.context_switches = 0;
    task_stats.total_cpu_time = 0;
    
    /* Initialize queues */
    ready_queue_head = NULL;
    blocked_queue_head = NULL;
    current_task = NULL;
    next_pid = 1;
    
    /* Create idle task (PID 0) */
    uint32_t idle_pid = task_create("idle", idle_task_main, NULL, 
                                    PRIORITY_IDLE, TASK_STACK_SIZE);
    if (idle_pid == 0) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to create idle task");
        return MEOW_ERROR_INITIALIZATION_FAILED;
    }
    
    /* Set idle task as current */
    current_task = task_get_by_pid(idle_pid);
    if (current_task) {
        current_task->state = TASK_STATE_RUNNING;
        remove_from_ready_queue(current_task);
    }
    
    meow_log(MEOW_LOG_CHIRP, "😺 Task system initialized with idle task (PID %u)", idle_pid);
    return MEOW_SUCCESS;
}

uint32_t task_create(const char* name, 
                     void (*entry_point)(void* arg),
                     void* arg,
                     task_priority_t priority,
                     size_t stack_size)
{
    MEOW_RETURN_VALUE_IF_NULL(name, 0);
    MEOW_RETURN_VALUE_IF_NULL(entry_point, 0);
    
    /* Find free task slot */
    task_t* task = find_free_task_slot();
    if (!task) {
        meow_log(MEOW_LOG_YOWL, "🙀 No free task slots available");
        return 0;
    }
    
    /* Set default stack size if not specified */
    if (stack_size == 0) {
        stack_size = TASK_STACK_SIZE;
    }
    
    /* Allocate territory using existing memory system */
    uint32_t territory_id = purr_alloc_territory();
    if (territory_id == 0) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to allocate territory for task");
        return 0;
    }
    
    /* Allocate stack using existing heap allocator */
    void* stack_base = meow_heap_alloc(stack_size);
    if (!stack_base) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to allocate stack for task");
        purr_free_territory(territory_id);
        return 0;
    }
    
    /* Allocate CPU context structure */
    cpu_context_t* context = (cpu_context_t*)meow_heap_alloc(sizeof(cpu_context_t));
    if (!context) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to allocate CPU context");
        meow_heap_free(stack_base);
        purr_free_territory(territory_id);
        return 0;
    }
    
    /* Initialize task structure */
    task->pid = next_pid++;
    meow_strncpy(task->name, name, TASK_NAME_LENGTH - 1, TASK_NAME_LENGTH);
    task->name[TASK_NAME_LENGTH - 1] = '\0';
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->time_slice = 10; /* Default 10ms time slice */
    task->total_runtime = 0;
    task->context = context;
    task->territory_id = territory_id;
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->stack_top = (void*)((char*)stack_base + stack_size);
    task->parent_pid = current_task ? current_task->pid : 0;
    task->child_pids = NULL;
    task->child_count = 0;
    task->creation_time = HAL_TIMER_OP(get_ticks);
    task->last_scheduled = 0;
    task->fd_table = NULL;
    task->entry_point = entry_point;
    task->entry_arg = arg;
    task->exit_code = 0;
    task->next = NULL;
    task->prev = NULL;
    
    /* Setup initial CPU context */
    if (task_setup_initial_context(task, entry_point, arg, task->stack_top) != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "🙀 Failed to setup initial context");
        task_free_resources(task);
        return 0;
    }
    
    /* Add to ready queue */
    add_to_ready_queue(task);
    
    /* Update statistics */
    task_stats.total_tasks++;
    task_stats.ready_tasks++;
    
    meow_log(MEOW_LOG_CHIRP, "😺 Created task '%s' with PID %u (territory: %u)", 
             name, task->pid, territory_id);
    
    return task->pid;
}

meow_error_t task_terminate(uint32_t pid, int exit_code)
{
    task_t* task = task_get_by_pid(pid);
    if (!task) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    if (task == current_task) {
        meow_log(MEOW_LOG_PURR, "😴 Current task terminating with exit code %d", exit_code);
    } else {
        meow_log(MEOW_LOG_PURR, "😴 Task '%s' (PID %u) terminated with exit code %d", 
                 task->name, pid, exit_code);
    }
    
    /* Set exit code and state */
    task->exit_code = exit_code;
    task->state = TASK_STATE_TERMINATED;
    
    /* Remove from queues */
    remove_from_ready_queue(task);
    remove_from_blocked_queue(task);
    
    /* Update statistics */
    if (task_stats.running_tasks > 0) task_stats.running_tasks--;
    if (task_stats.ready_tasks > 0) task_stats.ready_tasks--;
    if (task_stats.blocked_tasks > 0) task_stats.blocked_tasks--;
    task_stats.terminated_tasks++;
    
    /* If current task is terminating, yield to scheduler */
    if (task == current_task) {
        task_yield();
    }
    
    return MEOW_SUCCESS;
}

void task_terminate_self(int exit_code)
{
    task_t* current = task_get_current();
    if (current) {
        task_terminate(current->pid, exit_code);
    }
}


task_t* task_get_current(void)
{
    return current_task;
}

task_t* task_get_by_pid(uint32_t pid)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].state != TASK_STATE_UNUSED && task_table[i].pid == pid) {
            return &task_table[i];
        }
    }
    return NULL;
}

meow_error_t task_get_info(uint32_t pid, task_t* task_out)
{
    MEOW_RETURN_IF_NULL(task_out);
    
    task_t* task = task_get_by_pid(pid);
    if (!task) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    *task_out = *task; /* Copy task structure */
    return MEOW_SUCCESS;
}

/* ================================================================
 * TASK STATE MANAGEMENT
 * ================================================================ */

meow_error_t task_set_state(uint32_t pid, task_state_t new_state)
{
    task_t* task = task_get_by_pid(pid);
    if (!task) {
        return MEOW_ERROR_IO_FAILURE;
    }
    
    task_state_t old_state = task->state;
    task->state = new_state;
    
    /* Move between queues based on state change */
    switch (old_state) {
        case TASK_STATE_READY:
            remove_from_ready_queue(task);
            task_stats.ready_tasks--;
            break;
        case TASK_STATE_BLOCKED:
            remove_from_blocked_queue(task);
            task_stats.blocked_tasks--;
            break;
        case TASK_STATE_RUNNING:
            task_stats.running_tasks--;
            break;
        default:
            break;
    }
    
    switch (new_state) {
        case TASK_STATE_READY:
            add_to_ready_queue(task);
            task_stats.ready_tasks++;
            break;
        case TASK_STATE_BLOCKED:
            add_to_blocked_queue(task);
            task_stats.blocked_tasks++;
            break;
        case TASK_STATE_RUNNING:
            task_stats.running_tasks++;
            break;
        default:
            break;
    }
    
    return MEOW_SUCCESS;
}

meow_error_t task_block(uint32_t pid)
{
    return task_set_state(pid, TASK_STATE_BLOCKED);
}

meow_error_t task_unblock(uint32_t pid)
{
    return task_set_state(pid, TASK_STATE_READY);
}

void task_yield(void)
{
    /* This will be called by the scheduler */
    /* For now, just trigger a timer interrupt to invoke scheduler */
    __asm__ volatile ("int $0x20"); /* Timer interrupt */
}

void task_sleep(uint32_t milliseconds)
{
    if (!current_task) return;
    
    /* Block current task */
    task_set_state(current_task->pid, TASK_STATE_BLOCKED);
    
    /* Set wake-up time (simplified - in real implementation, use timer callbacks) */
    current_task->last_scheduled = HAL_TIMER_OP(get_ticks) + (milliseconds / 10);
    
    /* Yield to scheduler */
    task_yield();
}

/* ================================================================
 * CONTEXT SWITCHING
 * ================================================================ */

meow_error_t task_setup_initial_context(task_t* task, 
                                        void (*entry_point)(void* arg),
                                        void* arg,
                                        void* stack_top)
{
    cpu_context_t* ctx = task->context;
    
    /* Clear context */
    meow_memset(ctx, 0, sizeof(cpu_context_t));
    
    /* Set up initial register values */
    ctx->eip = (uint32_t)entry_point;          /* Entry point */
    ctx->esp = (uint32_t)stack_top - 16;       /* Stack pointer (leave space) */
    ctx->ebp = ctx->esp;                       /* Base pointer */
    ctx->eflags = 0x202;                       /* Enable interrupts */
    ctx->cs = 0x08;                            /* Kernel code segment */
    ctx->ss = 0x10;                            /* Kernel data segment */
    ctx->ds = ctx->es = ctx->fs = ctx->gs = 0x10; /* Data segments */
    
    /* Set up argument (simplified - push to stack) */
    uint32_t* stack_ptr = (uint32_t*)(ctx->esp);
    stack_ptr[-1] = (uint32_t)arg;             /* Push argument */
    stack_ptr[-2] = 0;                         /* Return address (should never return) */
    ctx->esp = (uint32_t)&stack_ptr[-2];
    
    return MEOW_SUCCESS;
}

/* ================================================================
 * STATISTICS AND DEBUGGING
 * ================================================================ */

meow_error_t task_get_statistics(task_statistics_t* stats)
{
    MEOW_RETURN_IF_NULL(stats);
    
    *stats = task_stats;
    return MEOW_SUCCESS;
}

uint32_t task_list_all(task_t* buffer, uint32_t max_tasks)
{
    if (!buffer || max_tasks == 0) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_TASKS && count < max_tasks; i++) {
        if (task_table[i].state != TASK_STATE_UNUSED) {
            buffer[count++] = task_table[i];
        }
    }
    
    return count;
}

void task_dump_info(uint32_t pid)
{
    if (pid == 0) {
        /* Dump all tasks */
        meow_log(MEOW_LOG_CHIRP, "🔄 Task List:");
        meow_log(MEOW_LOG_PURR, "PID  Name            State     Priority  Runtime");
        meow_log(MEOW_LOG_PURR, "---  ----            -----     --------  -------");
        
        for (uint32_t i = 0; i < MAX_TASKS; i++) {
            task_t* task = &task_table[i];
            if (task->state != TASK_STATE_UNUSED) {
                const char* state_str;
                switch (task->state) {
                    case TASK_STATE_READY: state_str = "READY"; break;
                    case TASK_STATE_RUNNING: state_str = "RUNNING"; break;
                    case TASK_STATE_BLOCKED: state_str = "BLOCKED"; break;
                    case TASK_STATE_TERMINATED: state_str = "TERMINATED"; break;
                    default: state_str = "UNKNOWN"; break;
                }
                
                meow_log(MEOW_LOG_PURR, "%3u  %-12s  %-8s  %8u  %7u", 
                         task->pid, task->name, state_str, 
                         task->priority, task->total_runtime);
            }
        }
    } else {
        /* Dump specific task */
        task_t* task = task_get_by_pid(pid);
        if (task) {
            meow_log(MEOW_LOG_CHIRP, "🔄 Task Info (PID %u):", pid);
            meow_log(MEOW_LOG_PURR, "  Name: %s", task->name);
            meow_log(MEOW_LOG_PURR, "  State: %u", task->state);
            meow_log(MEOW_LOG_PURR, "  Priority: %u", task->priority);
            meow_log(MEOW_LOG_PURR, "  Territory: %u", task->territory_id);
            meow_log(MEOW_LOG_PURR, "  Stack: %p - %p (%zu bytes)", 
                     task->stack_base, task->stack_top, task->stack_size);
        } else {
            meow_log(MEOW_LOG_HISS, "😾 Task PID %u not found", pid);
        }
    }
}

/* ================================================================
 * CLEANUP AND RESOURCE MANAGEMENT
 * ================================================================ */

void task_cleanup_terminated(void)
{
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        task_t* task = &task_table[i];
        if (task->state == TASK_STATE_TERMINATED) {
            meow_log(MEOW_LOG_PURR, "🧹 Cleaning up terminated task '%s' (PID %u)", 
                     task->name, task->pid);
            task_free_resources(task);
            task->state = TASK_STATE_UNUSED;
            task_stats.terminated_tasks--;
        }
    }
}

void task_free_resources(task_t* task)
{
    if (!task) return;
    
    /* Free CPU context */
    if (task->context) {
        meow_heap_free(task->context);
        task->context = NULL;
    }
    
    /* Free stack */
    if (task->stack_base) {
        meow_heap_free(task->stack_base);
        task->stack_base = NULL;
        task->stack_top = NULL;
    }
    
    /* Free territory using existing memory system */
    if (task->territory_id != 0) {
        purr_free_territory(task->territory_id);
        task->territory_id = 0;
    }
    
    /* Free file descriptor table (when implemented) */
    if (task->fd_table) {
        meow_heap_free(task->fd_table);
        task->fd_table = NULL;
    }
}

/* ================================================================
 * DEFAULT TASK FUNCTIONS
 * ================================================================ */

void idle_task_main(void* arg)
{
    meow_log(MEOW_LOG_PURR, "😴 Idle task started - ready for cat naps");
    
    while (1) {
        /* Halt CPU until next interrupt */
        HAL_CPU_OP(halt);
    }
}

void init_task_main(void* arg)
{
    meow_log(MEOW_LOG_CHIRP, "🐾 Init task started - setting up cat paradise");
    
    /* In a real OS, this would start system services */
    /* For now, just demonstrate task execution */
    
    int counter = 0;
    while (counter < 5) {
        meow_log(MEOW_LOG_MEOW, "🐱 Init task tick %d", counter);
        task_sleep(2000); /* Sleep for 2 seconds */
        counter++;
    }
    
    meow_log(MEOW_LOG_PURR, "😺 Init task completed initialization");
    
    /* Terminate when done */
    task_terminate(current_task->pid, 0);
}

/* ================================================================
 * INTERNAL FUNCTIONS
 * ================================================================ */

task_t* task_get_table_entry(uint32_t index)
{
    if (index >= MAX_TASKS) return NULL;
    return &task_table[index];
}

uint32_t task_allocate_pid(void)
{
    return next_pid++;
}

void task_free_pid(uint32_t pid)
{
    /* In a more sophisticated implementation, we'd track and reuse PIDs */
    /* For now, we just increment monotonically */
}