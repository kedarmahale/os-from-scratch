#ifndef MEOWKERNEL_X86_64_PMM_H
#define MEOWKERNEL_X86_64_PMM_H

#include <stdint.h>

typedef enum {
    PMM64_OK = 0,
    PMM64_ERR_NOT_INITIALIZED,
    PMM64_ERR_NO_MEMORY_MAP,
    PMM64_ERR_INVALID_MEMORY_MAP,
    PMM64_ERR_OUT_OF_MEMORY,
    PMM64_ERR_INVALID_ADDRESS,
    PMM64_ERR_DOUBLE_FREE
} pmm64_status_t;

pmm64_status_t pmm64_init(const void *multiboot_info);
void *pmm64_alloc_page(void);
pmm64_status_t pmm64_free_page(void *page);
uint64_t pmm64_total_pages(void);
uint64_t pmm64_free_page_count(void);

#ifdef MEOW_TEST_MODE
int pmm64_self_test(void);
#endif

#endif
