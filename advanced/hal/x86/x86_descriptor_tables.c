/* advanced/hal/x86/x86_descriptor_tables.c - Global Descriptor Table Implementation (Fixed)
 *
 * Clean GDT implementation with correct logging
 * Copyright (c) 2025 MeowKernel Project
 */

#include "x86_meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* GDT Entry Structure */
struct gdt_entry {
    uint16_t limit_low;     /* Lower 16 bits of limit */
    uint16_t base_low;      /* Lower 16 bits of base */
    uint8_t  base_middle;   /* Next 8 bits of base */
    uint8_t  access;        /* Access flags */
    uint8_t  granularity;   /* Granularity and upper 4 bits of limit */
    uint8_t  base_high;     /* Upper 8 bits of base */
} __attribute__((packed));

/* GDT Pointer Structure */
struct gdt_ptr {
    uint16_t limit;         /* Size of GDT */
    uint32_t base;          /* Base address of GDT */
} __attribute__((packed));

/* GDT Entries */
static struct gdt_entry gdt[5];
static struct gdt_ptr gdt_ptr;

/* ============================================================================
 * GDT IMPLEMENTATION (Fixed function names and logging)
 * ============================================================================ */

/* Set a GDT entry */
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low     = (base & 0xFFFF);
    gdt[num].base_middle  = (base >> 16) & 0xFF;
    gdt[num].base_high    = (base >> 24) & 0xFF;
    gdt[num].limit_low    = (limit & 0xFFFF);
    gdt[num].granularity  = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access       = access;
}

/* Initialize the GDT */
meow_error_t x86_gdt_init(void) {
    meow_log(MEOW_LOG_CHIRP, "x86: Initializing Global Descriptor Table");

    /* Setup GDT pointer */
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    /* NULL descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel code segment: Base=0, Limit=4GB, Access=0x9A, Granularity=0xCF */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel data segment: Base=0, Limit=4GB, Access=0x92, Granularity=0xCF */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* User code segment: Base=0, Limit=4GB, Access=0xFA, Granularity=0xCF */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* User data segment: Base=0, Limit=4GB, Access=0xF2, Granularity=0xCF */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Load the GDT */
    x86_gdt_flush((uint32_t)&gdt_ptr);

    meow_log(MEOW_LOG_CHIRP, "x86: GDT initialized with 5 segments");
    return MEOW_SUCCESS;
}

/* Set a specific GDT gate (for external use) */
meow_error_t x86_gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                               uint8_t access, uint8_t granularity) {
    if (num >= 5) {
        meow_log(MEOW_LOG_YOWL, "x86: Invalid GDT entry number: %u", num);
        return MEOW_ERROR_INVALID_PARAMETER;
    }

    gdt_set_gate(num, base, limit, access, granularity);
    meow_log(MEOW_LOG_MEOW, "x86: Set GDT gate %u (base: 0x%08x, limit: 0x%08x)", num, base, limit);
    return MEOW_SUCCESS;
}

/* Get GDT selector for a given index */
uint32_t x86_gdt_get_selector(uint32_t index) {
    if (index >= 5) {
        meow_log(MEOW_LOG_YOWL, "x86: Invalid GDT selector index: %u", index);
        return 0;
    }
    return index * 8; /* Each GDT entry is 8 bytes */
}