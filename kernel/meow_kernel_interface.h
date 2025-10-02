/* kernel/meow_kernel_interface.h - MeowKernel Centralized Error Codes
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_KERNEL_INTERFACE_H
#define MEOW_KERNEL_INTERFACE_H

#include "../advanced/mm/meow_memory_mapper.h"

// Multiboot magic numbers
#define MULTIBOOT_MAGIC 0x2BADB002

multiboot_info_t* get_multiboot_info(void);
uint32_t get_multiboot_magic(void);
uint8_t is_multiboot_info_valid(void);

#endif /* MEOW_KERNEL_INTERFACE_H */
