#ifndef MEOWKERNEL_X86_64_CONTEXT_H
#define MEOWKERNEL_X86_64_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

/*
 * Cooperative kernel context. The switcher owns exactly these registers;
 * interrupt entry remains responsible for the complete interrupt frame.
 *
 * The stack pointer points at the return address of the interrupted call, and
 * rip points at the label that resumes that call. A newly-created context
 * instead points at x86_64_context_start with a synthetic stack return slot.
 */
typedef struct __attribute__((aligned(16))) x86_64_context {
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} x86_64_context_t;

_Static_assert(offsetof(x86_64_context_t, rsp) == 0, "context rsp offset");
_Static_assert(offsetof(x86_64_context_t, rip) == 8, "context rip offset");
_Static_assert(offsetof(x86_64_context_t, rflags) == 16, "context flags offset");
_Static_assert(offsetof(x86_64_context_t, rbx) == 24, "context rbx offset");
_Static_assert(offsetof(x86_64_context_t, rbp) == 32, "context rbp offset");
_Static_assert(offsetof(x86_64_context_t, r12) == 40, "context r12 offset");
_Static_assert(offsetof(x86_64_context_t, r13) == 48, "context r13 offset");
_Static_assert(offsetof(x86_64_context_t, r14) == 56, "context r14 offset");
_Static_assert(offsetof(x86_64_context_t, r15) == 64, "context r15 offset");
_Static_assert(sizeof(x86_64_context_t) == 80, "context size");

typedef enum {
    CONTEXT64_OK = 0,
    CONTEXT64_ERR_INVALID_CONTEXT,
    CONTEXT64_ERR_INVALID_STACK,
    CONTEXT64_ERR_INVALID_ENTRY
} context64_status_t;

/* Save the current cooperative context and transfer execution to next. */
void x86_64_context_switch(x86_64_context_t *current,
                           const x86_64_context_t *next);

/* Prepare a context that starts entry(argument) on the supplied stack. */
context64_status_t x86_64_context_init(x86_64_context_t *context,
                                        void *stack_base,
                                        size_t stack_size,
                                        void (*entry)(void *argument),
                                        void *argument);

#ifdef MEOW_TEST_MODE
int x86_64_context_self_test(void);
#endif

#endif
