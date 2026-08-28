#ifndef MEOWKERNEL_X86_64_PAGING_H
#define MEOWKERNEL_X86_64_PAGING_H

#include <stdint.h>

#define X86_64_PAGE_PRESENT  (1ULL << 0)
#define X86_64_PAGE_WRITABLE (1ULL << 1)
#define X86_64_PAGE_USER     (1ULL << 2)
#define X86_64_PAGE_PWT      (1ULL << 3)
#define X86_64_PAGE_PCD      (1ULL << 4)
#define X86_64_PAGE_GLOBAL   (1ULL << 8)
#define X86_64_PAGE_NX       (1ULL << 63)

typedef enum {
    PAGING64_OK = 0,
    PAGING64_ERR_NOT_INITIALIZED,
    PAGING64_ERR_INVALID_ADDRESS,
    PAGING64_ERR_ALREADY_MAPPED,
    PAGING64_ERR_NOT_MAPPED,
    PAGING64_ERR_OUT_OF_MEMORY,
    PAGING64_ERR_UNSUPPORTED_HUGE_PAGE
} paging64_status_t;

paging64_status_t x86_64_paging_init(void);
paging64_status_t x86_64_paging_map(uint64_t virtual_address,
                                    uint64_t physical_address,
                                    uint64_t flags);
paging64_status_t x86_64_paging_unmap(uint64_t virtual_address,
                                      uint64_t *physical_address);
int x86_64_paging_is_mapped(uint64_t virtual_address);
paging64_status_t x86_64_paging_translate(uint64_t virtual_address,
                                          uint64_t *physical_address);

#ifdef MEOW_TEST_MODE
int x86_64_paging_self_test(void);
#endif

#endif
