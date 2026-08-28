#include <stddef.h>
#include <stdint.h>

#include "x86_64_scheduler.h"
#include "../../hal/x86_64/x86_64_interrupts.h"
#include "../../mm/x86_64_heap.h"

extern void x86_64_scheduler_start(void);

#define X86_64_SCHEDULER_MAX_TASKS 4
#define X86_64_DEFAULT_STACK_SIZE 8192
#define X86_64_TASK_STACK_HEADROOM 512
#define X86_64_CONTEXT_FRAME_SIZE (sizeof(x86_64_interrupt_frame_t))

typedef enum {
    TASK64_UNUSED = 0,
    TASK64_READY,
    TASK64_RUNNING,
    TASK64_TERMINATED
} task64_state_t;

typedef struct x86_64_scheduler_task {
    uint32_t pid;
    char name[24];
    task64_state_t state;
    void *stack_base;
    size_t stack_size;
    x86_64_task_entry_t entry;
    void *argument;
    x86_64_interrupt_frame_t *frame;
    uint64_t runtime_ticks;
} x86_64_scheduler_task_t;

static x86_64_scheduler_task_t tasks[X86_64_SCHEDULER_MAX_TASKS];
static x86_64_scheduler_task_t bootstrap_task;
static x86_64_scheduler_task_t *current_task;
static uint32_t next_pid;
static uint32_t next_slot;
static uint64_t scheduler_ticks;
static uint64_t switch_count;
static uint8_t initialized;

static uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    return value & ~(alignment - 1);
}

static void copy_name(char *destination, const char *source)
{
    uint32_t index;

    for (index = 0; index + 1 < sizeof(((x86_64_scheduler_task_t *)0)->name) &&
                  source[index]; ++index) {
        destination[index] = source[index];
    }
    destination[index] = '\0';
}

static x86_64_scheduler_task_t *find_free_task(void)
{
    for (uint32_t index = 0; index < X86_64_SCHEDULER_MAX_TASKS; ++index) {
        if (tasks[index].state == TASK64_UNUSED) return &tasks[index];
    }
    return NULL;
}

static uint32_t runnable_count(void)
{
    uint32_t count = 0;

    for (uint32_t index = 0; index < X86_64_SCHEDULER_MAX_TASKS; ++index) {
        if (tasks[index].state == TASK64_READY ||
            tasks[index].state == TASK64_RUNNING) ++count;
    }
    if (bootstrap_task.state == TASK64_RUNNING) ++count;
    return count;
}

static x86_64_scheduler_task_t *next_ready_task(void)
{
    for (uint32_t offset = 0; offset < X86_64_SCHEDULER_MAX_TASKS; ++offset) {
        uint32_t index = (next_slot + offset) % X86_64_SCHEDULER_MAX_TASKS;
        if (tasks[index].state == TASK64_READY) {
            next_slot = (index + 1) % X86_64_SCHEDULER_MAX_TASKS;
            return &tasks[index];
        }
    }
    if (bootstrap_task.state == TASK64_READY) return &bootstrap_task;
    return NULL;
}

scheduler64_status_t x86_64_scheduler_init(void)
{
    if (initialized) return SCHEDULER64_ERR_ALREADY_INITIALIZED;
    for (uint32_t index = 0; index < X86_64_SCHEDULER_MAX_TASKS; ++index) {
        tasks[index].state = TASK64_UNUSED;
    }
    bootstrap_task.pid = 0;
    bootstrap_task.state = TASK64_RUNNING;
    bootstrap_task.frame = NULL;
    current_task = &bootstrap_task;
    next_pid = 1;
    next_slot = 0;
    scheduler_ticks = 0;
    switch_count = 0;
    initialized = 1;
    return SCHEDULER64_OK;
}

uint32_t x86_64_scheduler_create(const char *name,
                                 x86_64_task_entry_t entry,
                                 void *argument,
                                 size_t stack_size)
{
    x86_64_scheduler_task_t *task;
    uintptr_t stack_end;
    uintptr_t frame_address;

    if (!initialized || !entry || !name) return 0;
    if (stack_size == 0) stack_size = X86_64_DEFAULT_STACK_SIZE;
    if (stack_size < 512) return 0;
    task = find_free_task();
    if (!task) return 0;
    task->stack_base = x86_64_heap_alloc(stack_size);
    if (!task->stack_base) return 0;
    task->stack_size = stack_size;
    if ((uintptr_t)task->stack_base > UINTPTR_MAX - stack_size) {
        x86_64_heap_free(task->stack_base);
        task->stack_base = NULL;
        return 0;
    }
    stack_end = align_down((uintptr_t)task->stack_base + stack_size, 16);
    if (stack_end < (uintptr_t)task->stack_base +
                    X86_64_TASK_STACK_HEADROOM +
                    X86_64_CONTEXT_FRAME_SIZE + 8) {
        x86_64_heap_free(task->stack_base);
        task->stack_base = NULL;
        return 0;
    }
    /* Leave space above the synthetic frame for the first IRQ and call. */
    frame_address = stack_end - X86_64_TASK_STACK_HEADROOM -
                    X86_64_CONTEXT_FRAME_SIZE;
    task->frame = (x86_64_interrupt_frame_t *)frame_address;
    for (uint32_t index = 0;
         index < sizeof(*task->frame) / sizeof(uint64_t); ++index) {
        ((uint64_t *)task->frame)[index] = 0;
    }
    task->pid = next_pid++;
    copy_name(task->name, name);
    task->entry = entry;
    task->argument = argument;
    task->state = TASK64_READY;
    task->runtime_ticks = 0;
    task->frame->rdi = (uintptr_t)argument;
    task->frame->rip = (uintptr_t)x86_64_scheduler_start;
    task->frame->r12 = (uintptr_t)entry;
    task->frame->r13 = (uintptr_t)argument;
    task->frame->cs = 0x08;
    task->frame->rflags = 0x246;
    return task->pid;
}

x86_64_interrupt_frame_t *x86_64_scheduler_timer_tick(
    x86_64_interrupt_frame_t *frame)
{
    x86_64_scheduler_task_t *next;

    if (!initialized || !frame) return frame;
    ++scheduler_ticks;
    current_task->frame = frame;
    ++current_task->runtime_ticks;
    next = next_ready_task();
    if (!next) return frame;
    if (current_task != &bootstrap_task &&
        current_task->state == TASK64_RUNNING) {
        current_task->state = TASK64_READY;
    } else if (current_task == &bootstrap_task) {
        current_task->state = TASK64_READY;
    }
    next->state = TASK64_RUNNING;
    current_task = next;
    ++switch_count;
    return next->frame;
}

scheduler64_status_t x86_64_scheduler_get_stats(x86_64_scheduler_stats_t *stats)
{
    if (!initialized) return SCHEDULER64_ERR_NOT_INITIALIZED;
    if (!stats) return SCHEDULER64_ERR_INVALID_STACK;
    stats->timer_ticks = scheduler_ticks;
    stats->context_switches = switch_count;
    stats->runnable_tasks = runnable_count();
    stats->current_pid = current_task ? current_task->pid : 0;
    return SCHEDULER64_OK;
}
