/* advanced/process/meow_task.h - tMeowKernel Process Management Interface
 *
 * Builds on existing memory territory system from Phase 1
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_TASK_H
#define MEOW_TASK_H

#include <stdint.h>
#include "../kernel/meow_error_definitions.h"
#include "../advanced/mm/meow_memory_manager.h"

/* ================================================================
 * PROCESS STATES AND CONSTANTS
 * ================================================================ */

#define MAX_TASKS           64
#define TASK_NAME_LENGTH    32
#define TASK_STACK_SIZE     4096
#define IDLE_TASK_PID       0
#define INIT_TASK_PID       1

/* Task states */
typedef enum task_state {
    TASK_STATE_UNUSED = 0,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_state_t;

/* Task priority levels */
typedef enum task_priority {
    PRIORITY_IDLE = 0,
    PRIORITY_LOW = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_HIGH = 3,
    PRIORITY_REALTIME = 4
} task_priority_t;

/* ================================================================
 * CPU CONTEXT STRUCTURE (Architecture-specific)
 * ================================================================ */

/* x86 CPU context for task switching */
typedef struct cpu_context {
    /* General purpose registers (saved by pusha/popa) */
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;
    
    /* Segment registers */
    uint32_t ds, es, fs, gs;
    
    /* Interrupt frame (saved by processor/interrupt handler) */
    uint32_t eip, cs, eflags, esp, ss;
} __attribute__((packed)) cpu_context_t;

/* ================================================================
 * TASK CONTROL BLOCK (TCB)
 * ================================================================ */

typedef struct task {
    /* Task identification */
    uint32_t pid;                       /* Process ID */
    char name[TASK_NAME_LENGTH];        /* Task name */
    
    /* Task state */
    task_state_t state;                 /* Current state */
    task_priority_t priority;           /* Priority level */
    uint32_t time_slice;                /* Remaining time slice */
    uint32_t total_runtime;             /* Total CPU time used */
    
    /* CPU context for task switching */
    cpu_context_t* context;             /* Saved CPU context */
    
    /* Memory management (uses existing territory system) */
    uint32_t territory_id;              /* Territory from existing system */
    void* stack_base;                   /* Stack base address */
    void* stack_top;                    /* Current stack pointer */
    size_t stack_size;                  /* Stack size */
    
    /* Task hierarchy */
    uint32_t parent_pid;                /* Parent process ID */
    uint32_t* child_pids;               /* Array of child PIDs */
    uint32_t child_count;               /* Number of children */
    
    /* Timing information */
    uint64_t creation_time;             /* When task was created */
    uint64_t last_scheduled;            /* Last time task was scheduled */
    
    /* File descriptors (for future file system support) */
    void* fd_table;                     /* File descriptor table */
    
    /* Task entry point and arguments */
    void (*entry_point)(void* arg);     /* Task main function */
    void* entry_arg;                    /* Argument to entry function */
    
    /* Exit information */
    int exit_code;                      /* Exit code when terminated */
    
    /* Linked list for scheduler */
    struct task* next;                  /* Next task in queue */
    struct task* prev;                  /* Previous task in queue */
    
} task_t;

/* ================================================================
 * TASK MANAGEMENT FUNCTIONS
 * ================================================================ */

/**
 * task_system_init - Initialize the task management system
 * 
 * Sets up task structures and creates the idle task
 * Uses existing memory management system for allocation
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_system_init(void);

/**
 * task_create - Create a new task
 * @name: Task name (max TASK_NAME_LENGTH-1 characters)
 * @entry_point: Function to execute
 * @arg: Argument to pass to entry_point
 * @priority: Task priority level
 * @stack_size: Stack size (0 for default)
 * 
 * Uses existing territory system for memory allocation
 * 
 * @return PID of new task, or 0 if failed
 */
uint32_t task_create(const char* name, 
                     void (*entry_point)(void* arg),
                     void* arg,
                     task_priority_t priority,
                     size_t stack_size);

/**
 * task_terminate - Terminate a task
 * @pid: Process ID to terminate
 * @exit_code: Exit code
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_terminate(uint32_t pid, int exit_code);

/**
 * task_get_current - Get current running task
 * 
 * @return Pointer to current task, or NULL if none
 */
task_t* task_get_current(void);

/**
 * task_get_by_pid - Get task by process ID
 * @pid: Process ID to find
 * 
 * @return Pointer to task, or NULL if not found
 */
task_t* task_get_by_pid(uint32_t pid);

/**
 * task_get_info - Get task information
 * @pid: Process ID
 * @task_out: Output task structure
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_get_info(uint32_t pid, task_t* task_out);

/* ================================================================
 * TASK STATE MANAGEMENT
 * ================================================================ */

/**
 * task_set_state - Change task state
 * @pid: Process ID
 * @new_state: New state to set
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_set_state(uint32_t pid, task_state_t new_state);

/**
 * task_block - Block a task (set to BLOCKED state)
 * @pid: Process ID to block
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_block(uint32_t pid);

/**
 * task_unblock - Unblock a task (set to READY state)
 * @pid: Process ID to unblock
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_unblock(uint32_t pid);

/**
 * task_yield - Voluntarily yield CPU to another task
 */
void task_yield(void);

/**
 * task_sleep - Put current task to sleep for specified time
 * @milliseconds: Time to sleep in milliseconds
 * 
 * Uses existing timer system for scheduling wakeup
 */
void task_sleep(uint32_t milliseconds);

/* ================================================================
 * CONTEXT SWITCHING (Architecture-specific)
 * ================================================================ */

/**
 * task_switch_context - Switch CPU context between tasks
 * @from: Current task (to save context)
 * @to: Next task (to restore context)
 * 
 * Architecture-specific assembly implementation
 */
void task_switch_context(task_t* from, task_t* to);

/**
 * task_setup_initial_context - Setup initial CPU context for new task
 * @task: Task to setup
 * @entry_point: Task entry function
 * @arg: Argument to pass to entry function
 * @stack_top: Top of task stack
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_setup_initial_context(task_t* task, 
                                        void (*entry_point)(void* arg),
                                        void* arg,
                                        void* stack_top);

/* ================================================================
 * TASK STATISTICS AND DEBUGGING
 * ================================================================ */

typedef struct task_statistics {
    uint32_t total_tasks;
    uint32_t running_tasks;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
    uint32_t terminated_tasks;
    uint32_t context_switches;
    uint64_t total_cpu_time;
} task_statistics_t;

/**
 * task_get_statistics - Get system-wide task statistics
 * @stats: Output statistics structure
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t task_get_statistics(task_statistics_t* stats);

/**
 * task_list_all - List all tasks in system
 * @buffer: Output buffer for task list
 * @max_tasks: Maximum number of tasks to return
 * 
 * @return Number of tasks returned
 */
uint32_t task_list_all(task_t* buffer, uint32_t max_tasks);

/**
 * task_dump_info - Dump detailed task information for debugging
 * @pid: Process ID (0 for all tasks)
 */
void task_dump_info(uint32_t pid);

/* ================================================================
 * TASK CLEANUP AND RESOURCE MANAGEMENT
 * ================================================================ */

/**
 * task_cleanup_terminated - Clean up terminated tasks
 * 
 * Frees resources of tasks in TERMINATED state
 * Called periodically by the scheduler
 */
void task_cleanup_terminated(void);

/**
 * task_free_resources - Free all resources used by a task
 * @task: Task to free resources for
 * 
 * Uses existing memory management system to free territories
 */
void task_free_resources(task_t* task);

/* ================================================================
 * INTERNAL FUNCTIONS (Do not call directly)
 * ================================================================ */

/* Task table access */
task_t* task_get_table_entry(uint32_t index);
uint32_t task_allocate_pid(void);
void task_free_pid(uint32_t pid);

/* Default task functions */
void idle_task_main(void* arg);
void init_task_main(void* arg);

void task_terminate_self(int exit_code);

#endif /* MEOW_TASK_H */