/* kernel/multiboot.h - Unified Multiboot Structures (Fixed)
 *
 * Single definition location for all multiboot structures to resolve conflicts
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOWKERNEL_MULTIBOOT_H
#define MEOWKERNEL_MULTIBOOT_H

#include <stdint.h>

/* ============================================================================
 * MULTIBOOT CONSTANTS
 * ============================================================================ */

#define MULTIBOOT_MAGIC         0x2BADB002
#define MULTIBOOT_HEADER_MAGIC  0x1BADB002

/* Multiboot flags */
#define MULTIBOOT_FLAG_MEM          (1 << 0)  /* Memory info available */
#define MULTIBOOT_FLAG_DEVICE       (1 << 1)  /* Boot device info available */
#define MULTIBOOT_FLAG_CMDLINE      (1 << 2)  /* Command line available */
#define MULTIBOOT_FLAG_MODS         (1 << 3)  /* Modules available */
#define MULTIBOOT_FLAG_SYMS         (1 << 4)  /* Symbol table info available */
#define MULTIBOOT_FLAG_MMAP         (1 << 6)  /* Memory map available */
#define MULTIBOOT_FLAG_DRIVES       (1 << 7)  /* Drives info available */
#define MULTIBOOT_FLAG_CONFIG       (1 << 8)  /* Config table available */
#define MULTIBOOT_FLAG_LOADER_NAME  (1 << 9)  /* Bootloader name available */
#define MULTIBOOT_FLAG_APM          (1 << 10) /* APM info available */
#define MULTIBOOT_FLAG_VBE          (1 << 11) /* VBE info available */

/* Memory map entry types */
#define MULTIBOOT_MMAP_TYPE_AVAILABLE   1     /* Available RAM */
#define MULTIBOOT_MMAP_TYPE_RESERVED    2     /* Reserved area */
#define MULTIBOOT_MMAP_TYPE_ACPI_RECL   3     /* ACPI reclaimable memory */
#define MULTIBOOT_MMAP_TYPE_NVS         4     /* ACPI NVS memory */
#define MULTIBOOT_MMAP_TYPE_BADRAM      5     /* Bad RAM */

/* ============================================================================
 * MULTIBOOT STRUCTURES (SINGLE DEFINITION POINT)
 * ============================================================================ */

/**
 * multiboot_info_t - Multiboot information structure
 * 
 * This is THE SINGLE definition of the multiboot info structure.
 * All other files must include this header instead of defining their own.
 */
typedef struct multiboot_info {
    uint32_t flags;                     /* Feature flags */
    uint32_t mem_lower;                 /* Lower memory (KB) */
    uint32_t mem_upper;                 /* Upper memory (KB) */
    uint32_t boot_device;               /* Boot device info */
    uint32_t cmdline;                   /* Command line pointer */
    uint32_t mods_count;                /* Number of modules */
    uint32_t mods_addr;                 /* Modules address */
    uint32_t syms[4];                   /* Symbol table info */
    uint32_t mmap_length;               /* Memory map length */
    uint32_t mmap_addr;                 /* Memory map address */
    uint32_t drives_length;             /* Drives length */
    uint32_t drives_addr;               /* Drives address */
    uint32_t config_table;              /* Config table */
    uint32_t boot_loader_name;          /* Bootloader name */
    uint32_t apm_table;                 /* APM table */
    uint32_t vbe_control_info;          /* VBE control info */
    uint32_t vbe_mode_info;             /* VBE mode info */
    uint16_t vbe_mode;                  /* VBE mode */
    uint16_t vbe_interface_seg;         /* VBE interface segment */
    uint16_t vbe_interface_off;         /* VBE interface offset */
    uint16_t vbe_interface_len;         /* VBE interface length */
} __attribute__((packed)) multiboot_info_t;

/**
 * multiboot_mmap_entry_t - Memory map entry structure
 * 
 * This is THE SINGLE definition of the memory map entry structure.
 */
typedef struct multiboot_mmap_entry {
    uint32_t size;                      /* Size of this entry (minus this field) */
    uint64_t addr;                      /* Starting address */
    uint64_t len;                       /* Length in bytes */
    uint32_t type;                      /* Type of memory region */
} __attribute__((packed)) multiboot_mmap_entry_t;

/**
 * multiboot_module_t - Module information structure
 */
typedef struct multiboot_module {
    uint32_t mod_start;                 /* Module start address */
    uint32_t mod_end;                   /* Module end address */
    uint32_t cmdline;                   /* Module command line */
    uint32_t pad;                       /* Padding for alignment */
} __attribute__((packed)) multiboot_module_t;

/* ============================================================================
 * MULTIBOOT UTILITY FUNCTIONS (Inline for performance)
 * ============================================================================ */

/**
 * multiboot_has_flag - Check if a specific flag is set
 */
static inline int multiboot_has_flag(const multiboot_info_t* mbi, uint32_t flag) {
    return mbi && (mbi->flags & flag);
}

/**
 * multiboot_has_memory_info - Check if memory info is available
 */
static inline int multiboot_has_memory_info(const multiboot_info_t* mbi) {
    return multiboot_has_flag(mbi, MULTIBOOT_FLAG_MEM);
}

/**
 * multiboot_has_memory_map - Check if memory map is available
 */
static inline int multiboot_has_memory_map(const multiboot_info_t* mbi) {
    return multiboot_has_flag(mbi, MULTIBOOT_FLAG_MMAP);
}

/**
 * multiboot_get_total_memory - Get total memory in bytes
 */
static inline uint64_t multiboot_get_total_memory(const multiboot_info_t* mbi) {
    if (!multiboot_has_memory_info(mbi)) {
        return 0;
    }
    /* Convert KB to bytes and add lower + upper memory */
    return ((uint64_t)mbi->mem_lower + (uint64_t)mbi->mem_upper) * 1024ULL;
}

/**
 * multiboot_get_memory_map - Get pointer to memory map
 */
static inline multiboot_mmap_entry_t* multiboot_get_memory_map(const multiboot_info_t* mbi) {
    if (!multiboot_has_memory_map(mbi)) {
        return NULL;
    }
    return (multiboot_mmap_entry_t*)mbi->mmap_addr;
}

/**
 * multiboot_mmap_entry_next - Get next memory map entry
 */
static inline multiboot_mmap_entry_t* multiboot_mmap_entry_next(multiboot_mmap_entry_t* entry) {
    return (multiboot_mmap_entry_t*)((char*)entry + entry->size + sizeof(entry->size));
}

/**
 * multiboot_mmap_is_available - Check if memory map entry is available RAM
 */
static inline int multiboot_mmap_is_available(const multiboot_mmap_entry_t* entry) {
    return entry && entry->type == MULTIBOOT_MMAP_TYPE_AVAILABLE;
}

#endif /* MEOWKERNEL_MULTIBOOT_H */