#include <stdint.h>
#include <stddef.h>

#include "x86_64_paging.h"
#include "x86_64_pmm.h"

#define PAGE_SIZE 4096ULL
#define PAGE_TABLE_ENTRIES 512ULL
#define PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define HUGE_PAGE_BIT (1ULL << 7)
#define ONE_GIB (1ULL << 30)
#define TWO_MIB (1ULL << 21)

static uint64_t *root_table;
static uint8_t initialized;

static inline uint64_t read_cr3(void)
{
    uint64_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static inline void invalidate_page(uint64_t address)
{
    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)address) : "memory");
}

static inline int is_canonical(uint64_t address)
{
    return address < 0x0000800000000000ULL ||
           address >= 0xFFFF800000000000ULL;
}

static inline uint64_t *table_from_entry(uint64_t entry)
{
    return (uint64_t *)(uintptr_t)(entry & PAGE_ADDRESS_MASK);
}

static uint64_t *allocate_table(void)
{
    void *page = pmm64_alloc_page();
    uint64_t *table;

    if (!page) return NULL;
    table = (uint64_t *)page;
    for (uint64_t i = 0; i < PAGE_TABLE_ENTRIES; ++i) table[i] = 0;
    return table;
}

static paging64_status_t walk_to_pte(uint64_t virtual_address,
                                     int create,
                                     uint64_t **pte)
{
    uint64_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_address >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;
    uint64_t *table;

    if (!root_table) return PAGING64_ERR_NOT_INITIALIZED;

    entry = root_table[pml4_index];
    if (!(entry & X86_64_PAGE_PRESENT)) {
        if (!create) return PAGING64_ERR_NOT_MAPPED;
        table = allocate_table();
        if (!table) return PAGING64_ERR_OUT_OF_MEMORY;
        root_table[pml4_index] = (uint64_t)(uintptr_t)table |
                                 X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE;
        entry = root_table[pml4_index];
    }
    if (entry & HUGE_PAGE_BIT) return PAGING64_ERR_UNSUPPORTED_HUGE_PAGE;
    pdpt = table_from_entry(entry);

    entry = pdpt[pdpt_index];
    if (!(entry & X86_64_PAGE_PRESENT)) {
        if (!create) return PAGING64_ERR_NOT_MAPPED;
        table = allocate_table();
        if (!table) return PAGING64_ERR_OUT_OF_MEMORY;
        pdpt[pdpt_index] = (uint64_t)(uintptr_t)table |
                           X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE;
        entry = pdpt[pdpt_index];
    }
    if (entry & HUGE_PAGE_BIT) return PAGING64_ERR_UNSUPPORTED_HUGE_PAGE;
    pd = table_from_entry(entry);

    entry = pd[pd_index];
    if (!(entry & X86_64_PAGE_PRESENT)) {
        if (!create) return PAGING64_ERR_NOT_MAPPED;
        table = allocate_table();
        if (!table) return PAGING64_ERR_OUT_OF_MEMORY;
        pd[pd_index] = (uint64_t)(uintptr_t)table |
                       X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE;
        entry = pd[pd_index];
    }
    if (entry & HUGE_PAGE_BIT) return PAGING64_ERR_UNSUPPORTED_HUGE_PAGE;
    pt = table_from_entry(entry);
    *pte = &pt[pt_index];
    return PAGING64_OK;
}

paging64_status_t x86_64_paging_init(void)
{
    root_table = (uint64_t *)(uintptr_t)(read_cr3() & PAGE_ADDRESS_MASK);
    initialized = root_table != NULL;
    return initialized ? PAGING64_OK : PAGING64_ERR_NOT_INITIALIZED;
}

paging64_status_t x86_64_paging_map(uint64_t virtual_address,
                                    uint64_t physical_address,
                                    uint64_t flags)
{
    uint64_t *pte;
    paging64_status_t status;

    if (!initialized) return PAGING64_ERR_NOT_INITIALIZED;
    if (!is_canonical(virtual_address) ||
        virtual_address % PAGE_SIZE || physical_address % PAGE_SIZE) {
        return PAGING64_ERR_INVALID_ADDRESS;
    }
    status = walk_to_pte(virtual_address, 1, &pte);
    if (status != PAGING64_OK) return status;
    if (*pte & X86_64_PAGE_PRESENT) return PAGING64_ERR_ALREADY_MAPPED;
    *pte = (physical_address & PAGE_ADDRESS_MASK) |
           (flags & (X86_64_PAGE_WRITABLE | X86_64_PAGE_USER |
                     X86_64_PAGE_PWT | X86_64_PAGE_PCD |
                     X86_64_PAGE_GLOBAL | X86_64_PAGE_NX)) |
           X86_64_PAGE_PRESENT;
    invalidate_page(virtual_address);
    return PAGING64_OK;
}

paging64_status_t x86_64_paging_unmap(uint64_t virtual_address,
                                      uint64_t *physical_address)
{
    uint64_t *pte;
    paging64_status_t status;

    if (!initialized || !is_canonical(virtual_address) ||
        virtual_address % PAGE_SIZE) return PAGING64_ERR_INVALID_ADDRESS;
    status = walk_to_pte(virtual_address, 0, &pte);
    if (status != PAGING64_OK) return status;
    if (!(*pte & X86_64_PAGE_PRESENT)) return PAGING64_ERR_NOT_MAPPED;
    if (physical_address) *physical_address = *pte & PAGE_ADDRESS_MASK;
    *pte = 0;
    invalidate_page(virtual_address);
    return PAGING64_OK;
}

int x86_64_paging_is_mapped(uint64_t virtual_address)
{
    uint64_t physical_address;
    return x86_64_paging_translate(virtual_address, &physical_address) == PAGING64_OK;
}

paging64_status_t x86_64_paging_translate(uint64_t virtual_address,
                                          uint64_t *physical_address)
{
    uint64_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_address >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;
    uint64_t entry;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    if (!initialized) return PAGING64_ERR_NOT_INITIALIZED;
    if (!physical_address || !is_canonical(virtual_address)) return PAGING64_ERR_INVALID_ADDRESS;

    entry = root_table[pml4_index];
    if (!(entry & X86_64_PAGE_PRESENT)) return PAGING64_ERR_NOT_MAPPED;
    pdpt = table_from_entry(entry);
    entry = pdpt[pdpt_index];
    if (!(entry & X86_64_PAGE_PRESENT)) return PAGING64_ERR_NOT_MAPPED;
    if (entry & HUGE_PAGE_BIT) {
        *physical_address = (entry & 0x000FFFFFC0000000ULL) + (virtual_address & (ONE_GIB - 1));
        return PAGING64_OK;
    }
    pd = table_from_entry(entry);
    entry = pd[pd_index];
    if (!(entry & X86_64_PAGE_PRESENT)) return PAGING64_ERR_NOT_MAPPED;
    if (entry & HUGE_PAGE_BIT) {
        *physical_address = (entry & 0x000FFFFFFFE00000ULL) + (virtual_address & (TWO_MIB - 1));
        return PAGING64_OK;
    }
    pt = table_from_entry(entry);
    entry = pt[pt_index];
    if (!(entry & X86_64_PAGE_PRESENT)) return PAGING64_ERR_NOT_MAPPED;
    *physical_address = (entry & PAGE_ADDRESS_MASK) + (virtual_address & (PAGE_SIZE - 1));
    return PAGING64_OK;
}

#ifdef MEOW_TEST_MODE
int x86_64_paging_self_test(void)
{
    const uint64_t virtual_address = 0x40000000ULL;
    const uint64_t value = 0x4D454F5750414745ULL;
    uint64_t physical_address;
    uint64_t unmapped_physical;
    uint64_t *mapped;
    void *page;

    if (x86_64_paging_init() != PAGING64_OK ||
        x86_64_paging_is_mapped(virtual_address) ||
        x86_64_paging_map(virtual_address + 1, 0, X86_64_PAGE_WRITABLE) !=
            PAGING64_ERR_INVALID_ADDRESS) return 0;
    page = pmm64_alloc_page();
    if (!page || x86_64_paging_map(virtual_address, (uintptr_t)page,
                                   X86_64_PAGE_WRITABLE) != PAGING64_OK) return 0;
    if (x86_64_paging_map(virtual_address, (uintptr_t)page,
                          X86_64_PAGE_WRITABLE) != PAGING64_ERR_ALREADY_MAPPED) return 0;
    if (!x86_64_paging_is_mapped(virtual_address) ||
        x86_64_paging_translate(virtual_address, &physical_address) != PAGING64_OK ||
        physical_address != (uintptr_t)page) return 0;
    mapped = (uint64_t *)(uintptr_t)virtual_address;
    *mapped = value;
    if (*mapped != value ||
        x86_64_paging_unmap(virtual_address, &unmapped_physical) != PAGING64_OK ||
        unmapped_physical != (uintptr_t)page ||
        x86_64_paging_is_mapped(virtual_address) ||
        x86_64_paging_map(virtual_address, (uintptr_t)page,
                          X86_64_PAGE_WRITABLE) != PAGING64_OK ||
        x86_64_paging_unmap(virtual_address, NULL) != PAGING64_OK ||
        pmm64_free_page(page) != PMM64_OK) return 0;
    return 1;
}
#endif
