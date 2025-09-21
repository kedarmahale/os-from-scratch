#ifndef MEOW_KERNEL_H
#define MEOW_KERNEL_H

#include "../advanced/mm/territory_map.h"

// Multiboot magic numbers
#define MULTIBOOT_MAGIC 0x2BADB002

multiboot_info_t* get_multiboot_info(void);
uint32_t get_multiboot_magic(void);
uint8_t is_multiboot_info_valid(void);
uint8_t validate_multiboot_info(uint32_t magic, multiboot_info_t* mbi);

#endif /* MEOW_KERNEL_H */
