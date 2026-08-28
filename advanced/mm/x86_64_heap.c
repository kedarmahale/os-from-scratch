#include <stdint.h>
#include <stddef.h>

#include "x86_64_heap.h"
#include "x86_64_pmm.h"
#include "x86_64_paging.h"

#define PAGE_SIZE 4096ULL
#define BLOCK_MAGIC 0x4D454F5748454150ULL /* MEOWHEAP */
#define BLOCK_FREE 1ULL
#define BLOCK_ALLOCATED 0ULL

typedef struct __attribute__((aligned(16))) x86_64_heap_block {
    uint64_t size;
    uint64_t state;
    uint64_t magic;
    struct x86_64_heap_block *next;
    struct x86_64_heap_block *previous;
} x86_64_heap_block_t;

static x86_64_heap_block_t *first_block;
static uint64_t mapped_end;
static uint64_t used_bytes;
static uint64_t allocation_count;
static uint64_t deallocation_count;
static uint64_t failure_count;
static uint64_t free_block_count;
static uint64_t allocated_block_count;
static uint8_t initialized;

static inline uint64_t align_up(uint64_t value)
{
    return (value + X86_64_HEAP_ALIGNMENT - 1) & ~(X86_64_HEAP_ALIGNMENT - 1);
}

static inline uint64_t page_align_up(uint64_t value)
{
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static int normalize_size(size_t requested_size, uint64_t *normalized_size)
{
    uint64_t capacity = X86_64_HEAP_SIZE - sizeof(x86_64_heap_block_t);

    if (requested_size == 0 || (uint64_t)requested_size > capacity) return 0;
    *normalized_size = align_up((uint64_t)requested_size);
    return 1;
}

static inline uint64_t irq_save(void)
{
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint64_t flags)
{
    __asm__ volatile ("pushq %0; popfq" : : "r"(flags) : "memory", "cc");
}

static inline void *block_payload(x86_64_heap_block_t *block)
{
    return (void *)((uint8_t *)block + sizeof(*block));
}

static inline x86_64_heap_block_t *payload_block(const void *pointer)
{
    return (x86_64_heap_block_t *)((uint8_t *)pointer - sizeof(x86_64_heap_block_t));
}

static int map_heap_pages(uint64_t start, uint64_t end)
{
    uint64_t mapped_start = page_align_up(start);
    uint64_t mapped_target = page_align_up(end);
    uint64_t current;

    for (current = mapped_start; current < mapped_target; current += PAGE_SIZE) {
        void *physical = pmm64_alloc_page();
        if (!physical || x86_64_paging_map(current, (uintptr_t)physical,
                                           X86_64_PAGE_WRITABLE) != PAGING64_OK) {
            if (physical) pmm64_free_page(physical);
            while (current != mapped_start) {
                uint64_t old_physical;
                current -= PAGE_SIZE;
                if (x86_64_paging_unmap(current, &old_physical) == PAGING64_OK) {
                    pmm64_free_page((void *)(uintptr_t)old_physical);
                }
            }
            return 0;
        }
    }
    mapped_end = mapped_target;
    return 1;
}

static x86_64_heap_block_t *find_free_block(uint64_t size)
{
    x86_64_heap_block_t *block = first_block;
    while (block) {
        if (block->magic != BLOCK_MAGIC) return NULL;
        if (block->state == BLOCK_FREE && block->size >= size) return block;
        block = block->next;
    }
    return NULL;
}

static int pointer_belongs_to_block(const void *pointer,
                                   x86_64_heap_block_t **result)
{
    x86_64_heap_block_t *block = first_block;
    uintptr_t address = (uintptr_t)pointer;

    if (!pointer || address < X86_64_HEAP_BASE + sizeof(*block) ||
        address >= X86_64_HEAP_BASE + X86_64_HEAP_SIZE) return 0;
    while (block) {
        if (block->magic != BLOCK_MAGIC) return 0;
        if (block_payload(block) == pointer) {
            *result = block;
            return 1;
        }
        block = block->next;
    }
    return 0;
}

static void split_block(x86_64_heap_block_t *block, uint64_t size)
{
    x86_64_heap_block_t *remainder;

    if (block->size < size + sizeof(*block) + X86_64_HEAP_ALIGNMENT) return;
    remainder = (x86_64_heap_block_t *)((uint8_t *)block + sizeof(*block) + size);
    remainder->size = block->size - size - sizeof(*block);
    remainder->state = BLOCK_FREE;
    remainder->magic = BLOCK_MAGIC;
    remainder->next = block->next;
    remainder->previous = block;
    if (remainder->next) remainder->next->previous = remainder;
    block->next = remainder;
    block->size = size;
    ++free_block_count;
}

static void merge_with_next(x86_64_heap_block_t *block)
{
    x86_64_heap_block_t *next = block->next;
    if (!next || next->state != BLOCK_FREE) return;
    block->size += sizeof(*block) + next->size;
    block->next = next->next;
    if (block->next) block->next->previous = block;
    if (free_block_count) --free_block_count;
}

heap64_status_t x86_64_heap_init(void)
{
    uint64_t flags;

    if (initialized) return HEAP64_ERR_ALREADY_INITIALIZED;
    flags = irq_save();
    mapped_end = X86_64_HEAP_BASE;
    if (!map_heap_pages(X86_64_HEAP_BASE,
                        X86_64_HEAP_BASE + X86_64_HEAP_SIZE)) {
        irq_restore(flags);
        return HEAP64_ERR_OUT_OF_MEMORY;
    }
    first_block = (x86_64_heap_block_t *)(uintptr_t)X86_64_HEAP_BASE;
    first_block->size = X86_64_HEAP_SIZE - sizeof(*first_block);
    first_block->state = BLOCK_FREE;
    first_block->magic = BLOCK_MAGIC;
    first_block->next = NULL;
    first_block->previous = NULL;
    used_bytes = 0;
    allocation_count = 0;
    deallocation_count = 0;
    failure_count = 0;
    free_block_count = 1;
    allocated_block_count = 0;
    initialized = 1;
    irq_restore(flags);
    return HEAP64_OK;
}

void *x86_64_heap_alloc(size_t requested_size)
{
    uint64_t size;
    uint64_t flags;
    x86_64_heap_block_t *block;

    if (!initialized || requested_size == 0) {
        ++failure_count;
        return NULL;
    }
    if (!normalize_size(requested_size, &size)) {
        ++failure_count;
        return NULL;
    }
    flags = irq_save();
    block = find_free_block(size);
    if (!block) {
        ++failure_count;
        irq_restore(flags);
        return NULL;
    }
    split_block(block, size);
    block->state = BLOCK_ALLOCATED;
    used_bytes += block->size;
    ++allocation_count;
    ++allocated_block_count;
    if (free_block_count) --free_block_count;
    irq_restore(flags);
    return block_payload(block);
}

heap64_status_t x86_64_heap_free(void *pointer)
{
    uint64_t flags;
    x86_64_heap_block_t *block;

    if (!initialized) return HEAP64_ERR_NOT_INITIALIZED;
    if (!pointer) return HEAP64_OK;
    flags = irq_save();
    if (!pointer_belongs_to_block(pointer, &block)) {
        irq_restore(flags);
        return HEAP64_ERR_INVALID_POINTER;
    }
    if (block->state == BLOCK_FREE) {
        irq_restore(flags);
        return HEAP64_ERR_DOUBLE_FREE;
    }
    if (block->size > used_bytes) {
        irq_restore(flags);
        return HEAP64_ERR_CORRUPTED;
    }
    block->state = BLOCK_FREE;
    used_bytes -= block->size;
    ++deallocation_count;
    if (allocated_block_count) --allocated_block_count;
    ++free_block_count;
    if (block->next && block->next->state == BLOCK_FREE) merge_with_next(block);
    if (block->previous && block->previous->state == BLOCK_FREE) {
        merge_with_next(block->previous);
    }
    irq_restore(flags);
    return HEAP64_OK;
}

void *x86_64_heap_realloc(void *pointer, size_t size)
{
    x86_64_heap_block_t *block;
    void *replacement;
    size_t copy_size;
    uint64_t normalized_size;

    if (!pointer) return x86_64_heap_alloc(size);
    if (size == 0) {
        x86_64_heap_free(pointer);
        return NULL;
    }
    if (!pointer_belongs_to_block(pointer, &block) || block->state != BLOCK_ALLOCATED) return NULL;
    if (!normalize_size(size, &normalized_size)) return NULL;
    if (block->size >= normalized_size) return pointer;
    replacement = x86_64_heap_alloc(size);
    if (!replacement) return NULL;
    copy_size = block->size < size ? (size_t)block->size : size;
    for (size_t i = 0; i < copy_size; ++i) ((uint8_t *)replacement)[i] = ((uint8_t *)pointer)[i];
    x86_64_heap_free(pointer);
    return replacement;
}

void *x86_64_heap_calloc(size_t count, size_t size)
{
    size_t total;
    uint8_t *memory;

    if (count && size > SIZE_MAX / count) return NULL;
    total = count * size;
    memory = (uint8_t *)x86_64_heap_alloc(total);
    if (!memory) return NULL;
    for (size_t i = 0; i < total; ++i) memory[i] = 0;
    return memory;
}

heap64_status_t x86_64_heap_get_stats(x86_64_heap_stats_t *stats)
{
    if (!initialized) return HEAP64_ERR_NOT_INITIALIZED;
    if (!stats) return HEAP64_ERR_INVALID_POINTER;
    stats->capacity = X86_64_HEAP_SIZE - sizeof(x86_64_heap_block_t);
    stats->used = used_bytes;
    stats->free = stats->capacity - used_bytes;
    stats->allocations = allocation_count;
    stats->deallocations = deallocation_count;
    stats->failures = failure_count;
    stats->free_blocks = free_block_count;
    stats->allocated_blocks = allocated_block_count;
    return HEAP64_OK;
}

#ifdef MEOW_TEST_MODE
int x86_64_heap_self_test(void)
{
    x86_64_heap_stats_t before;
    x86_64_heap_stats_t after;
    uint8_t *first;
    uint8_t *second;
    uint8_t *reused;
    uint8_t *third;
    uint8_t *large;
    uint8_t *zeroed;

    if ((!initialized && x86_64_heap_init() != HEAP64_OK) ||
        x86_64_heap_get_stats(&before) != HEAP64_OK) return 0;
    first = (uint8_t *)x86_64_heap_alloc(24);
    second = (uint8_t *)x86_64_heap_alloc(4096);
    third = (uint8_t *)x86_64_heap_alloc(64);
    if (!first || !second || !third ||
        ((uintptr_t)first % X86_64_HEAP_ALIGNMENT) ||
        ((uintptr_t)second % X86_64_HEAP_ALIGNMENT) ||
        x86_64_heap_free(second) != HEAP64_OK) return 0;
    reused = (uint8_t *)x86_64_heap_alloc(128);
    if (!reused || reused != second || x86_64_heap_free(reused) != HEAP64_OK ||
        x86_64_heap_free(second) != HEAP64_ERR_DOUBLE_FREE) return 0;
    for (uint32_t i = 0; i < 24; ++i) first[i] = (uint8_t)i;
    first = (uint8_t *)x86_64_heap_realloc(first, 128);
    if (!first) return 0;
    for (uint32_t i = 0; i < 24; ++i) if (first[i] != (uint8_t)i) return 0;
    if (x86_64_heap_free(third) != HEAP64_OK) return 0;
    large = (uint8_t *)x86_64_heap_alloc(8 * 1024 * 1024);
    if (!large) return 0;
    large[0] = 0xA5;
    large[(8 * 1024 * 1024) - 1] = 0x5A;
    zeroed = (uint8_t *)x86_64_heap_calloc(128, 8);
    if (!zeroed) return 0;
    for (uint32_t i = 0; i < 128 * 8; ++i) if (zeroed[i] != 0) return 0;
    if (x86_64_heap_alloc(X86_64_HEAP_SIZE) != NULL ||
        x86_64_heap_free((void *)((uintptr_t)zeroed + 1)) != HEAP64_ERR_INVALID_POINTER) return 0;
    if (x86_64_heap_free(first) != HEAP64_OK ||
        x86_64_heap_free(large) != HEAP64_OK ||
        x86_64_heap_free(zeroed) != HEAP64_OK ||
        x86_64_heap_get_stats(&after) != HEAP64_OK ||
        after.used != 0 || after.free != before.free) return 0;
    return 1;
}
#endif
