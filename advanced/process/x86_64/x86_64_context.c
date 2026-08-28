#include <stddef.h>
#include <stdint.h>

#include "x86_64_context.h"
#include "../../mm/x86_64_heap.h"

extern void x86_64_context_start(void);

static uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    return value & ~(alignment - 1);
}

context64_status_t x86_64_context_init(x86_64_context_t *context,
                                        void *stack_base,
                                        size_t stack_size,
                                        void (*entry)(void *argument),
                                        void *argument)
{
    uintptr_t base;
    uintptr_t end;
    uintptr_t initial_stack;

    if (!context) return CONTEXT64_ERR_INVALID_CONTEXT;
    if (!entry) return CONTEXT64_ERR_INVALID_ENTRY;
    if (!stack_base || stack_size < 32) return CONTEXT64_ERR_INVALID_STACK;

    base = (uintptr_t)stack_base;
    if (base > UINTPTR_MAX - stack_size) return CONTEXT64_ERR_INVALID_STACK;
    end = align_down(base + stack_size, 16);
    if (end < base || end - base < 16) return CONTEXT64_ERR_INVALID_STACK;

    /* The synthetic return slot makes the initial entry stack ABI-shaped. */
    initial_stack = end - 8;
    *(uint64_t *)initial_stack = 0;

    for (size_t i = 0; i < sizeof(*context) / sizeof(uint64_t); ++i) {
        ((uint64_t *)context)[i] = 0;
    }
    context->rsp = initial_stack;
    context->rip = (uintptr_t)x86_64_context_start;
    context->rflags = 0x2; /* reserved bit; scheduling enables IF later */
    context->r12 = (uintptr_t)entry;
    context->r13 = (uintptr_t)argument;
    return CONTEXT64_OK;
}

#ifdef MEOW_TEST_MODE
typedef struct context_test_task {
    x86_64_context_t context;
    x86_64_context_t *return_context;
    x86_64_context_t *peer_context;
    volatile uint32_t *turn;
    volatile uint32_t *counter;
    volatile uint32_t *failures;
    uint32_t id;
    uint32_t rounds;
} context_test_task_t;

static void context_test_task(void *argument)
{
    context_test_task_t *task = (context_test_task_t *)argument;

    for (uint32_t round = 0; round < task->rounds; ++round) {
        if (*task->turn != task->id) ++*task->failures;
        ++*task->counter;
        *task->turn = task->id == 1 ? 2 : 1;
        x86_64_context_switch(&task->context, task->peer_context);
    }
    x86_64_context_switch(&task->context, task->return_context);
}

int x86_64_context_self_test(void)
{
    enum { ROUNDS = 8, STACK_SIZE = 4096 };
    x86_64_context_t bootstrap = {0};
    context_test_task_t first_task;
    context_test_task_t second_task;
    void *first_stack;
    void *second_stack;
    volatile uint32_t turn = 1;
    volatile uint32_t first_count = 0;
    volatile uint32_t second_count = 0;
    volatile uint32_t failures = 0;
    int result;

    first_stack = x86_64_heap_alloc(STACK_SIZE);
    second_stack = x86_64_heap_alloc(STACK_SIZE);
    if (!first_stack || !second_stack) return 0;

    first_task.return_context = &bootstrap;
    first_task.peer_context = &second_task.context;
    first_task.turn = &turn;
    first_task.counter = &first_count;
    first_task.failures = &failures;
    first_task.id = 1;
    first_task.rounds = ROUNDS;
    second_task.return_context = &bootstrap;
    second_task.peer_context = &first_task.context;
    second_task.turn = &turn;
    second_task.counter = &second_count;
    second_task.failures = &failures;
    second_task.id = 2;
    second_task.rounds = ROUNDS;

    if (x86_64_context_init(&first_task.context, first_stack, STACK_SIZE,
                            context_test_task, &first_task) != CONTEXT64_OK ||
        x86_64_context_init(&second_task.context, second_stack, STACK_SIZE,
                            context_test_task, &second_task) != CONTEXT64_OK) {
        x86_64_heap_free(first_stack);
        x86_64_heap_free(second_stack);
        return 0;
    }

    x86_64_context_switch(&bootstrap, &first_task.context);
    result = failures == 0 && turn == 1 && first_count == ROUNDS &&
             second_count == ROUNDS;
    x86_64_heap_free(first_stack);
    x86_64_heap_free(second_stack);
    return result;
}
#endif
