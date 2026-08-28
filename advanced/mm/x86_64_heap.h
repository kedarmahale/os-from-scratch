#ifndef MEOWKERNEL_X86_64_HEAP_H
#define MEOWKERNEL_X86_64_HEAP_H

#include <stdint.h>
#include <stddef.h>

#define X86_64_HEAP_BASE       0x0000000200000000ULL /* 8 GiB */
#define X86_64_HEAP_SIZE       (16ULL * 1024 * 1024)
#define X86_64_HEAP_ALIGNMENT  16ULL

typedef enum {
    HEAP64_OK = 0,
    HEAP64_ERR_NOT_INITIALIZED,
    HEAP64_ERR_ALREADY_INITIALIZED,
    HEAP64_ERR_OUT_OF_MEMORY,
    HEAP64_ERR_INVALID_SIZE,
    HEAP64_ERR_INVALID_POINTER,
    HEAP64_ERR_DOUBLE_FREE,
    HEAP64_ERR_CORRUPTED
} heap64_status_t;

typedef struct x86_64_heap_stats {
    uint64_t capacity;
    uint64_t used;
    uint64_t free;
    uint64_t allocations;
    uint64_t deallocations;
    uint64_t failures;
    uint64_t free_blocks;
    uint64_t allocated_blocks;
} x86_64_heap_stats_t;

heap64_status_t x86_64_heap_init(void);
void *x86_64_heap_alloc(size_t size);
heap64_status_t x86_64_heap_free(void *pointer);
void *x86_64_heap_realloc(void *pointer, size_t size);
void *x86_64_heap_calloc(size_t count, size_t size);
heap64_status_t x86_64_heap_get_stats(x86_64_heap_stats_t *stats);

#ifdef MEOW_TEST_MODE
int x86_64_heap_self_test(void);
#endif

#endif
