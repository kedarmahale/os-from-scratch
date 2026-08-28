#include <stdint.h>
#include <stddef.h>

#include "x86_64_pmm.h"
#include "../../kernel/meow_multiboot.h"

#define PAGE_SIZE              4096ULL
#define MAX_PAGES              (1ULL << 18) /* first 1 GiB */
#define MAX_PHYSICAL_ADDRESS   (MAX_PAGES * PAGE_SIZE)
#define BOOT_RESERVED_END      (2ULL * 1024 * 1024)
#define BITMAP_WORDS            (MAX_PAGES / 64)

/* A set bit in used_bitmap means reserved or allocated. */
static uint64_t used_bitmap[BITMAP_WORDS];
/* available_bitmap identifies RAM that the allocator is allowed to return. */
static uint64_t available_bitmap[BITMAP_WORDS];
/* A separate bit distinguishes an allocated page from a reserved page. */
static uint64_t allocated_bitmap[BITMAP_WORDS];
static uint64_t total_pages;
static uint64_t free_pages;
static uint64_t next_hint;
static uint8_t initialized;

extern char _kernel_start[];
extern char _kernel_end[];

static inline uint64_t page_align_up(uint64_t address)
{
    return (address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static inline uint64_t page_align_down(uint64_t address)
{
    return address & ~(PAGE_SIZE - 1);
}

static inline int bit_is_set(const uint64_t *bitmap, uint64_t page)
{
    return (bitmap[page / 64] & (1ULL << (page % 64))) != 0;
}

static inline void bit_set(uint64_t *bitmap, uint64_t page)
{
    bitmap[page / 64] |= 1ULL << (page % 64);
}

static inline void bit_clear(uint64_t *bitmap, uint64_t page)
{
    bitmap[page / 64] &= ~(1ULL << (page % 64));
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

static uint64_t memory_map_end(const multiboot_mmap_entry_t *entry)
{
    if (entry->len > UINT64_MAX - entry->addr) return UINT64_MAX;
    return entry->addr + entry->len;
}

static int next_memory_map_entry(const multiboot_info_t *mbi,
                                 uint32_t *offset,
                                 multiboot_mmap_entry_t **entry)
{
    multiboot_mmap_entry_t *current;

    if (!mbi || !(mbi->flags & MULTIBOOT_FLAG_MMAP) ||
        !mbi->mmap_addr || !mbi->mmap_length) return 0;
    if (*offset >= mbi->mmap_length ||
        mbi->mmap_length - *offset < sizeof(uint32_t)) return 0;

    current = (multiboot_mmap_entry_t *)((uintptr_t)mbi->mmap_addr + *offset);
    if (current->size < 20 ||
        current->size > mbi->mmap_length - *offset - sizeof(uint32_t)) return 0;

    *entry = current;
    *offset += sizeof(uint32_t) + current->size;
    return 1;
}

static void mark_free_range(uint64_t start, uint64_t end)
{
    uint64_t first;
    uint64_t last;

    if (start >= MAX_PHYSICAL_ADDRESS || end <= start) return;
    if (end > MAX_PHYSICAL_ADDRESS) end = MAX_PHYSICAL_ADDRESS;
    first = page_align_up(start) / PAGE_SIZE;
    last = page_align_down(end) / PAGE_SIZE;
    if (last > total_pages) last = total_pages;
    for (uint64_t page = first; page < last; ++page) {
        bit_set(available_bitmap, page);
        if (bit_is_set(used_bitmap, page)) {
            bit_clear(used_bitmap, page);
            ++free_pages;
        }
    }
}

static void mark_reserved_range(uint64_t start, uint64_t end)
{
    uint64_t first;
    uint64_t last;

    if (start >= MAX_PHYSICAL_ADDRESS || end <= start) return;
    if (end > MAX_PHYSICAL_ADDRESS) end = MAX_PHYSICAL_ADDRESS;
    first = page_align_down(start) / PAGE_SIZE;
    last = page_align_up(end) / PAGE_SIZE;
    if (last > total_pages) last = total_pages;
    for (uint64_t page = first; page < last; ++page) {
        bit_clear(available_bitmap, page);
        if (!bit_is_set(used_bitmap, page)) {
            bit_set(used_bitmap, page);
            if (free_pages) --free_pages;
        }
        bit_clear(allocated_bitmap, page);
    }
}

static void reserve_multiboot_data(const multiboot_info_t *mbi)
{
    uint64_t end;

    if (!mbi) return;
    mark_reserved_range((uintptr_t)mbi, (uintptr_t)mbi + sizeof(*mbi));

    if ((mbi->flags & MULTIBOOT_FLAG_MMAP) && mbi->mmap_addr && mbi->mmap_length) {
        end = (uint64_t)mbi->mmap_addr + mbi->mmap_length;
        mark_reserved_range(mbi->mmap_addr, end);
    }

    if ((mbi->flags & MULTIBOOT_FLAG_MODS) && mbi->mods_addr && mbi->mods_count) {
        end = (uint64_t)mbi->mods_addr +
              (uint64_t)mbi->mods_count * sizeof(multiboot_module_t);
        mark_reserved_range(mbi->mods_addr, end);
        for (uint32_t i = 0; i < mbi->mods_count; ++i) {
            const multiboot_module_t *module =
                &((const multiboot_module_t *)(uintptr_t)mbi->mods_addr)[i];
            mark_reserved_range(module->mod_start, module->mod_end);
        }
    }
}

pmm64_status_t pmm64_init(const void *multiboot_info)
{
    const multiboot_info_t *mbi = (const multiboot_info_t *)multiboot_info;
    uint32_t offset;
    uint64_t highest_address = 0;
    multiboot_mmap_entry_t *entry;

    initialized = 0;
    total_pages = 0;
    free_pages = 0;
    next_hint = 0;
    for (uint64_t i = 0; i < BITMAP_WORDS; ++i) {
        used_bitmap[i] = UINT64_MAX;
        available_bitmap[i] = 0;
        allocated_bitmap[i] = 0;
    }

    if (!mbi || !(mbi->flags & MULTIBOOT_FLAG_MMAP) ||
        !mbi->mmap_addr || !mbi->mmap_length) return PMM64_ERR_NO_MEMORY_MAP;

    offset = 0;
    while (offset < mbi->mmap_length) {
        if (!next_memory_map_entry(mbi, &offset, &entry)) {
            return PMM64_ERR_INVALID_MEMORY_MAP;
        }
        if (memory_map_end(entry) > highest_address) highest_address = memory_map_end(entry);
    }
    total_pages = highest_address / PAGE_SIZE;
    if (highest_address > MAX_PHYSICAL_ADDRESS) total_pages = MAX_PAGES;
    if (!total_pages) return PMM64_ERR_NO_MEMORY_MAP;

    /* First admit only type-1 RAM, then apply all reserved regions. */
    offset = 0;
    while (offset < mbi->mmap_length) {
        if (!next_memory_map_entry(mbi, &offset, &entry)) return PMM64_ERR_INVALID_MEMORY_MAP;
        if (entry->type == MULTIBOOT_MMAP_TYPE_AVAILABLE) {
            mark_free_range(entry->addr, memory_map_end(entry));
        }
    }
    offset = 0;
    while (offset < mbi->mmap_length) {
        if (!next_memory_map_entry(mbi, &offset, &entry)) return PMM64_ERR_INVALID_MEMORY_MAP;
        if (entry->type != MULTIBOOT_MMAP_TYPE_AVAILABLE) {
            mark_reserved_range(entry->addr, memory_map_end(entry));
        }
    }

    /* Protect low memory, the kernel image, and Multiboot-owned structures. */
    mark_reserved_range(0, BOOT_RESERVED_END);
    mark_reserved_range((uintptr_t)_kernel_start, (uintptr_t)_kernel_end);
    reserve_multiboot_data(mbi);
    initialized = 1;
    return free_pages ? PMM64_OK : PMM64_ERR_OUT_OF_MEMORY;
}

void *pmm64_alloc_page(void)
{
    uint64_t flags;
    uint64_t page;

    if (!initialized || !free_pages) return NULL;
    flags = irq_save();
    for (uint64_t i = 0; i < total_pages; ++i) {
        page = (next_hint + i) % total_pages;
        if (!bit_is_set(used_bitmap, page)) {
            bit_set(used_bitmap, page);
            bit_set(allocated_bitmap, page);
            --free_pages;
            next_hint = (page + 1) % total_pages;
            irq_restore(flags);
            return (void *)(uintptr_t)(page * PAGE_SIZE);
        }
    }
    irq_restore(flags);
    return NULL;
}

pmm64_status_t pmm64_free_page(void *page_address)
{
    uint64_t address = (uintptr_t)page_address;
    uint64_t page;
    uint64_t flags;

    if (!initialized) return PMM64_ERR_NOT_INITIALIZED;
    if (!page_address || address % PAGE_SIZE || address >= MAX_PHYSICAL_ADDRESS) {
        return PMM64_ERR_INVALID_ADDRESS;
    }
    page = address / PAGE_SIZE;
    if (page >= total_pages || !bit_is_set(available_bitmap, page)) {
        return PMM64_ERR_INVALID_ADDRESS;
    }
    if (!bit_is_set(allocated_bitmap, page)) return PMM64_ERR_DOUBLE_FREE;

    flags = irq_save();
    bit_clear(used_bitmap, page);
    bit_clear(allocated_bitmap, page);
    ++free_pages;
    irq_restore(flags);
    return PMM64_OK;
}

uint64_t pmm64_total_pages(void)
{
    return total_pages;
}

uint64_t pmm64_free_page_count(void)
{
    return free_pages;
}

#ifdef MEOW_TEST_MODE
static void *pmm_test_pages[MAX_PAGES];

int pmm64_self_test(void)
{
    uint64_t free_before = pmm64_free_page_count();
    uint64_t count = 0;
    void *first;
    void *second;
    void *reused;

    first = pmm64_alloc_page();
    second = pmm64_alloc_page();
    if (!first || !second || first == second ||
        ((uintptr_t)first % PAGE_SIZE) || ((uintptr_t)second % PAGE_SIZE)) return 0;
    if (pmm64_free_page(first) != PMM64_OK ||
        pmm64_free_page(first) != PMM64_ERR_DOUBLE_FREE ||
        pmm64_free_page((void *)((uintptr_t)second + 1)) != PMM64_ERR_INVALID_ADDRESS ||
        pmm64_free_page((void *)PAGE_SIZE) != PMM64_ERR_INVALID_ADDRESS) return 0;
    reused = pmm64_alloc_page();
    if (!reused || pmm64_free_page(second) != PMM64_OK ||
        pmm64_free_page(reused) != PMM64_OK) return 0;

    while (count < MAX_PAGES && (pmm_test_pages[count] = pmm64_alloc_page())) ++count;
    if (count != free_before || pmm64_alloc_page() != NULL ||
        pmm64_free_page_count() != 0 || !count) return 0;

    if (pmm64_free_page(pmm_test_pages[0]) != PMM64_OK ||
        pmm64_alloc_page() != pmm_test_pages[0]) return 0;
    if (pmm64_free_page(pmm_test_pages[0]) != PMM64_OK) return 0;
    for (uint64_t i = 1; i < count; ++i) {
        if (pmm64_free_page(pmm_test_pages[i]) != PMM64_OK) return 0;
    }
    return pmm64_free_page_count() == free_before;
}
#endif
