/* advanced/hal/x86/x86_platform_support.c - x86 Platform Support Functions
 *
 * C implementations of missing x86 HAL functions
 * Copyright (c) 2025 MeowKernel Project
 */

#include "x86_meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* ============================================================================
 * MEMORY DETECTION FUNCTIONS
 * ============================================================================ */

/**
 * x86_detect_memory_from_multiboot - Detect memory size from multiboot info
 */
uint32_t x86_detect_memory_from_multiboot(const multiboot_info_t* mbi) {
    if (!mbi) {
        meow_log(MEOW_LOG_HISS, "x86: NULL multiboot info for memory detection");
        return 0;
    }

    uint32_t total_memory = 0;

    /* Method 1: Basic memory info (if available) */
    if (mbi->flags & (1 << 0)) {
        /* mem_lower is memory below 1MB in KB */
        /* mem_upper is memory above 1MB in KB */
        total_memory = (mbi->mem_lower + mbi->mem_upper) * 1024;
        meow_log(MEOW_LOG_CHIRP, "x86: Basic memory: %u KB lower, %u KB upper",
                 mbi->mem_lower, mbi->mem_upper);
    }

    /* Method 2: Memory map (more detailed, preferred) */
    if (mbi->flags & (1 << 6) && mbi->mmap_addr && mbi->mmap_length) {
        meow_log(MEOW_LOG_CHIRP, "x86: Parsing memory map (%u bytes)", mbi->mmap_length);
        
        uint32_t mmap_total = 0;
        uint32_t entries = 0;
        
        for (uint32_t offset = 0; offset < mbi->mmap_length; ) {
            multiboot_mmap_entry_t* entry = (multiboot_mmap_entry_t*)(mbi->mmap_addr + offset);
            
            /* Validate entry */
            if (entry->size < 20) break; /* Minimum entry size */
            
            meow_log(MEOW_LOG_PURR, "x86: Memory region: 0x%08x%08x + %u MB (type %u)",
                     (uint32_t)(entry->addr >> 32), (uint32_t)(entry->addr & 0xFFFFFFFF),
                     (uint32_t)(entry->len / (1024 * 1024)), entry->type);
            
            /* Count available memory (type 1) */
            if (entry->type == 1) {
                mmap_total += (uint32_t)entry->len;
            }
            
            entries++;
            offset += entry->size + 4; /* Size field itself is not included in size */
            
            /* Safety limit */
            if (entries > 50) {
                meow_log(MEOW_LOG_HISS, "x86: Too many memory map entries, stopping");
                break;
            }
        }
        
        if (mmap_total > total_memory) {
            total_memory = mmap_total;
            meow_log(MEOW_LOG_CHIRP, "x86: Using memory map total: %u MB", total_memory / (1024 * 1024));
        }
    }

    /* Fallback if no memory detected */
    if (total_memory == 0) {
        total_memory = 64 * 1024 * 1024; /* 64MB fallback */
        meow_log(MEOW_LOG_HISS, "x86: No memory detected, using 64MB fallback");
    }

    meow_log(MEOW_LOG_CHIRP, "x86: Total system memory: %u MB", total_memory / (1024 * 1024));
    return total_memory;
}

/**
 * x86_get_kernel_memory_usage - Estimate kernel memory usage
 */
uint32_t x86_get_kernel_memory_usage(void) {
    /* Simple estimation based on typical kernel layout */
    extern uint32_t kernel_end;   /* Defined in linker script */
    extern uint32_t kernel_start; /* Defined in linker script */
    
    /* Calculate kernel size from linker symbols */
    uint32_t kernel_size = (uint32_t)&kernel_end - (uint32_t)&kernel_start;
    
    /* Add overhead for stacks, heaps, and data structures */
    uint32_t overhead = 2 * 1024 * 1024; /* 2MB overhead */
    uint32_t total_usage = kernel_size + overhead;
    
    meow_log(MEOW_LOG_CHIRP, "x86: Kernel memory usage: %u KB (kernel: %u KB + overhead: %u KB)",
             total_usage / 1024, kernel_size / 1024, overhead / 1024);
    
    return total_usage;
}

/* ============================================================================
 * CPU INFORMATION FUNCTIONS  
 * ============================================================================ */

/**
 * x86_get_cpu_vendor - Get CPU vendor string using CPUID
 */
const char* x86_get_cpu_vendor(void) {
    static char vendor_string[13] = {0}; /* 12 chars + null terminator */
    
    if (!x86_cpuid_supported()) {
        meow_log(MEOW_LOG_HISS, "x86: CPUID not supported");
        return "Unknown";
    }
    
    uint32_t eax, ebx, ecx, edx;
    
    /* CPUID function 0 returns vendor string */
    x86_cpuid(0, &eax, &ebx, &ecx, &edx);
    
    /* Vendor string is stored in EBX, EDX, ECX (in that order) */
    *((uint32_t*)(vendor_string + 0)) = ebx;
    *((uint32_t*)(vendor_string + 4)) = edx;
    *((uint32_t*)(vendor_string + 8)) = ecx;
    vendor_string[12] = '\0';
    
    meow_log(MEOW_LOG_CHIRP, "x86: CPU vendor: %s", vendor_string);
    return vendor_string;
}

/* ============================================================================
 * VGA TEXT OUTPUT FUNCTIONS
 * ============================================================================ */

/**
 * x86_vga_putc - Put character to VGA text mode display
 */
void x86_vga_putc(char c) {
    static uint16_t* vga_buffer = (uint16_t*)0xB8000;
    static uint8_t cursor_x = 0;
    static uint8_t cursor_y = 0;
    static uint8_t color = 0x07; /* Light gray on black */
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7; /* Align to 8-character boundary */
    } else if (c >= ' ') {
        uint16_t entry = (uint16_t)c | ((uint16_t)color << 8);
        vga_buffer[cursor_y * 80 + cursor_x] = entry;
        cursor_x++;
    }
    
    /* Handle screen edge */
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    
    /* Handle screen bottom (simple scroll) */
    if (cursor_y >= 25) {
        /* Scroll screen up */
        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 80; x++) {
                vga_buffer[y * 80 + x] = vga_buffer[(y + 1) * 80 + x];
            }
        }
        
        /* Clear last line */
        for (int x = 0; x < 80; x++) {
            vga_buffer[24 * 80 + x] = ((uint16_t)color << 8) | ' ';
        }
        
        cursor_y = 24;
    }
}

/* ============================================================================
 * TIMER FUNCTIONS
 * ============================================================================ */

/**
 * x86_pit_set_frequency - Set PIT timer frequency
 */
meow_error_t x86_pit_set_frequency(uint32_t frequency) {
    if (frequency == 0 || frequency > 1193180) {
        meow_log(MEOW_LOG_YOWL, "x86: Invalid PIT frequency: %u", frequency);
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    /* Calculate divisor */
    uint32_t divisor = 1193180 / frequency;
    if (divisor > 65535) {
        divisor = 65535;
        frequency = 1193180 / divisor;
        meow_log(MEOW_LOG_HISS, "x86: PIT frequency clamped to %u Hz", frequency);
    }
    
    meow_log(MEOW_LOG_CHIRP, "x86: Setting PIT frequency to %u Hz (divisor: %u)", frequency, divisor);
    
    /* Send command to PIT */
    x86_outb(0x43, 0x36); /* Channel 0, lobyte/hibyte, mode 3, binary */
    x86_outb(0x40, divisor & 0xFF);        /* Low byte */
    x86_outb(0x40, (divisor >> 8) & 0xFF); /* High byte */
    
    return MEOW_SUCCESS;
}

/* ============================================================================
 * INTERRUPT HANDLING
 * ============================================================================ */

/**
 * interrupt_handler - C interrupt handler called from assembly
 */
void interrupt_handler(void) {
    /* Simple interrupt handler - just acknowledge for now */
    meow_log(MEOW_LOG_PURR, "x86: Interrupt received");
    
    /* Send EOI to PIC for hardware interrupts */
    x86_outb(0x20, 0x20); /* EOI to master PIC */
}

/* ============================================================================
 * STUB FUNCTIONS FOR COMPATIBILITY
 * ============================================================================ */

/**
 * Additional stub functions that might be referenced
 */

/* Kernel symbols (usually defined in linker script) */
uint32_t kernel_start __attribute__((weak)) = 0x100000; /* 1MB */
uint32_t kernel_end __attribute__((weak)) = 0x200000;   /* 2MB */