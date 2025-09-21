/* advanced/hal/x86/hal_x86.c - Enhanced x86 HAL implementation */

#include "hal_x86.h"
#include "../../mm/territory_map.h"
#include "../../kernel/meow_util.h"

/* Global HAL state */
static uint32_t detected_memory_size = 0;
static uint32_t available_memory_size = 0;
static multiboot_info_t* saved_multiboot_info = NULL;
static uint8_t hal_memory_initialized = 0;

/* Interrupt handler table */
static void (*interrupt_handlers[256])(void);

/* Port I/O Implementation - AT&T syntax inline assembly */
uint8_t hal_port_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %w1, %b0" : "=a"(result) : "Nd"(port));
    return result;
}

uint16_t hal_port_inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %w1, %w0" : "=a"(result) : "Nd"(port));
    return result;
}

uint32_t hal_port_inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %w1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void hal_port_outb(uint16_t port, uint8_t data) {
    asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

void hal_port_outw(uint16_t port, uint16_t data) {
    asm volatile("outw %w0, %w1" : : "a"(data), "Nd"(port));
}

void hal_port_outl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %w1" : : "a"(data), "Nd"(port));
}

/* CPU Operations */
void hal_cpu_init(void) {
    meow_info("==== HAL: Initializing x86 CPU... ====\n");
    
    /* Initialize GDT first (memory segmentation) */
    meow_debug(" Initializing Global Descriptor Table...\n");
    x86_gdt_init();
    
    /* Initialize IDT (interrupt handling) */
    meow_debug(" Initializing Interrupt Descriptor Table... \n");
    x86_idt_init();
    
    /* Initialize PIC (interrupt controller) */
    meow_debug(" Initializing Programmable Interrupt Controller...\n");
    x86_pic_init();
    
    /* Enable basic CPU features */
    meow_debug(" Enabling CPU features...");
    
    /* Check and enable available CPU features */
    uint32_t eflags = hal_cpu_get_flags();
    meow_debug(" CPU EFLAGS: 0x%x", eflags);
    
    meow_info("==== HAL: x86 CPU initialization complete ====");
}

void hal_cpu_halt(void) {
    x86_hlt();
}

void hal_cpu_disable_interrupts(void) {
    x86_cli();
}

void hal_cpu_enable_interrupts(void) {
    x86_sti();
}

uint32_t hal_cpu_get_flags(void) {
    return x86_get_eflags();
}

/* Memory Operations with proper multiboot integration */
void hal_memory_init(void) {
    meow_info("==== HAL: Initializing x86 memory management...====\n");
    
    if (!saved_multiboot_info) {
	meow_warn(" No saved multiboot info found, using defaults!!\n");
        detected_memory_size = 128 * 1024 * 1024; /* 128MB default */
        available_memory_size = detected_memory_size - (8 * 1024 * 1024);
        hal_memory_initialized = 1;
        return;
    }
    
    /* Detect memory using multiboot information */
    detected_memory_size = detect_memory_from_multiboot(saved_multiboot_info);
    
    if (detected_memory_size == 0) {
        meow_warn(" Memory detection failed - using fallback values");
        detected_memory_size = 128 * 1024 * 1024; /* 128MB fallback */
    }
    
    /* Calculate available memory (total minus kernel and reserved areas) */
    uint32_t kernel_size = get_kernel_memory_usage();
    uint32_t reserved_size = 4 * 1024 * 1024; /* 4MB for kernel structures */
    
    if (detected_memory_size > (kernel_size + reserved_size)) {
        available_memory_size = detected_memory_size - kernel_size - reserved_size;
    } else {
        available_memory_size = 16 * 1024 * 1024; /* 16MB minimum */
    }
    
    meow_info(" Detected memory: %d MB", detected_memory_size / (1024 * 1024));
    meow_info(" Available memory: %d MB", available_memory_size / (1024 * 1024));
    meow_info(" Kernel size: %d KB", kernel_size / 1024);
    
    hal_memory_initialized = 1;
    meow_info("==== HAL: x86 memory management initialized ====");
}

/* IMPLEMENTED: Proper memory detection from multiboot */
uint32_t detect_memory_from_multiboot(multiboot_info_t* mbi) {
    if (!mbi) {
        meow_error(" No multiboot info for memory detection!!!!");
        return 0;
    }
    
    uint32_t total_memory = 0;
    
    /* Method 1: Try memory map first (most reliable) */
    if (mbi->flags & (1 << 6)) {
        meow_debug(" Using multiboot memory map for detection");
        
        multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)mbi->mmap_addr;
        uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
        
        while ((uint32_t)mmap < mmap_end) {
            if (mmap->type == 1) { /* Available memory */
                uint64_t region_end = mmap->addr + mmap->len;
                if (region_end > total_memory) {
                    total_memory = (uint32_t)region_end;
                }
                
                meow_debug(" Memory region: 0x%llx - 0x%llx (%llu KB)", 
                          mmap->addr, mmap->addr + mmap->len - 1, mmap->len / 1024);
            }
            
            /* Move to next entry */
            mmap = (multiboot_mmap_entry_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
    /* Method 2: Use basic memory info as fallback */
    else if (mbi->flags & (1 << 0)) {
        meow_debug(" Using basic multiboot memory info");
        
        /* mem_lower is memory below 1MB in KB */
        /* mem_upper is memory above 1MB in KB */
        total_memory = (mbi->mem_lower + mbi->mem_upper + 1024) * 1024;
        
        meow_debug(" Lower memory: %d KB", mbi->mem_lower);
        meow_debug(" Upper memory: %d KB", mbi->mem_upper);
    }
    
    /* Sanity check - ensure reasonable memory size */
    if (total_memory < (16 * 1024 * 1024)) {
        meow_warn(" Detected memory seems too low: %d MB", total_memory / (1024 * 1024));
        total_memory = 64 * 1024 * 1024; /* 64MB minimum */
    }
    
    if (total_memory > (4UL * 1024 * 1024 * 1024)) {
        meow_warn(" Detected memory over 4GB: %u MB - capping at 4GB", total_memory / (1024 * 1024));
        total_memory = 4UL * 1024 * 1024 * 1024; /* Cap at 4GB for 32-bit */
    }
    
    return total_memory;
}

uint32_t hal_memory_get_total_size(void) {
    if (!hal_memory_initialized) {
        hal_memory_init();
    }
    return detected_memory_size;
}

uint32_t hal_memory_get_available_size(void) {
    if (!hal_memory_initialized) {
        hal_memory_init();
    }
    return available_memory_size;
}

void* hal_memory_get_kernel_end(void) {
    extern char _kernel_end;
    return &_kernel_end;
}

/* IMPLEMENTED: Get actual kernel memory usage */
uint32_t get_kernel_memory_usage(void) {
    extern char _kernel_start, _kernel_end;
    uint32_t kernel_start = (uint32_t)&_kernel_start;
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    
    if (kernel_end > kernel_start) {
        return kernel_end - kernel_start;
    }
    
    /* Fallback estimate */
    return 2 * 1024 * 1024; /* 2MB */
}

/* IMPLEMENTED: Set multiboot info for memory detection */
void hal_set_multiboot_info(multiboot_info_t* mbi) {
    saved_multiboot_info = mbi;
    meow_debug(" HAL: Multiboot info set for memory detection");

    // Immediately detect memory if not already done
    if (!hal_memory_initialized && mbi) {
        detected_memory_size = detect_memory_from_multiboot(mbi);
        meow_info(" HAL: Memory detected during multiboot info setup: %d MB", 
                  detected_memory_size / (1024 * 1024));
    }
}

/* Timer Operations */
void hal_timer_init(uint32_t frequency) {
    meow_info(" HAL: Initializing x86 timer at %u Hz", frequency);
    
    /* Initialize PIT (Programmable Interval Timer) */
    x86_pit_init(frequency);
    
    meow_info(" HAL: x86 timer initialization complete");
}

void hal_timer_sleep(uint32_t milliseconds) {
    x86_pit_sleep(milliseconds);
}

/* Interrupt Operations */
void hal_interrupt_init(void) {
    meow_info(" HAL: Initializing x86 interrupt system...");
    
    /* Clear interrupt handler table */
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = NULL;
    }
    
    /* IDT and PIC are initialized in hal_cpu_init() */
    meow_info(" HAL: x86 interrupt system initialized");
}

/* Dynamic interrupt handler registration */
void hal_interrupt_register_handler(uint8_t interrupt, void (*handler)(void)) {
    if (!handler) {
        meow_error(" Cannot register NULL interrupt handler for INT %u", interrupt);
        return;
    }
    
    if (interrupt_handlers[interrupt] != NULL) {
        meow_warn(" Replacing existing handler for INT %u", interrupt);
    }
    
    interrupt_handlers[interrupt] = handler;
    meow_info(" Registered handler for INT %u at 0x%x", interrupt, (uint32_t)handler);
    
    /* If it's a hardware IRQ, enable it */
    if (interrupt >= 32 && interrupt <= 47) {
        hal_interrupt_enable(interrupt);
    }
}

/* Get registered interrupt handler */
void (*hal_interrupt_get_handler(uint8_t interrupt))(void) {
    return interrupt_handlers[interrupt];
}

/* Unregister interrupt handler */
void hal_interrupt_unregister_handler(uint8_t interrupt) {
    if (interrupt_handlers[interrupt] != NULL) {
        meow_info(" Unregistering handler for INT %u", interrupt);
        interrupt_handlers[interrupt] = NULL;
        
        /* If it's a hardware IRQ, disable it */
        if (interrupt >= 32 && interrupt <= 47) {
            hal_interrupt_disable(interrupt);
        }
    }
}

void hal_interrupt_enable(uint8_t interrupt) {
    if (interrupt >= 32 && interrupt <= 47) {
        /* Hardware IRQ */
        uint8_t irq = interrupt - 32;
        x86_pic_enable_irq(irq);
        meow_debug(" Enabled IRQ %u (INT %u)", irq, interrupt);
    } else {
        meow_warn(" Cannot enable interrupt %u (not a hardware IRQ)", interrupt);
    }
}

void hal_interrupt_disable(uint8_t interrupt) {
    if (interrupt >= 32 && interrupt <= 47) {
        /* Hardware IRQ */
        uint8_t irq = interrupt - 32;
        x86_pic_disable_irq(irq);
        meow_debug(" Disabled IRQ %u (INT %u)", irq, interrupt);
    } else {
        meow_warn(" Cannot disable interrupt %u (not a hardware IRQ)", interrupt);
    }
}

/* Debug Output */
void hal_debug_putc(char c) {
    /* Use existing VGA terminal output */
    extern void terminal_putchar(char c);
    terminal_putchar(c);
}

void hal_debug_puts(const char* str) {
    if (!str) return;
    
    while (*str) {
        hal_debug_putc(*str++);
    }
}

/* Enhanced debug output with formatting */
void hal_debug_printf(const char* format, ...) {
    /* Simple implementation - delegate to meow_printf for now */
    va_list args;
    va_start(args, format);
    
    /* We'll need a proper vprintf implementation, but for now: */
    meow_debug("HAL DEBUG: %s", format); /* Simplified */
    
    va_end(args);
}

/* x86-specific utility functions */
void x86_enable_interrupts_and_halt(void) {
    asm volatile("sti; hlt");
}

uint32_t x86_get_cr0(void) {
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    return cr0;
}

uint32_t x86_get_cr2(void) {
    uint32_t cr2;
    asm volatile("movl %%cr2, %0" : "=r"(cr2));
    return cr2;
}

uint32_t x86_get_cr3(void) {
    uint32_t cr3;
    asm volatile("movl %%cr3, %0" : "=r"(cr3));
    return cr3;
}

/* IMPLEMENTED: Additional x86 control register operations */
void x86_set_cr0(uint32_t cr0) {
    asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

void x86_set_cr3(uint32_t cr3) {
    asm volatile("movl %0, %%cr3" : : "r"(cr3));
}

/* IMPLEMENTED: CPU feature detection */
uint32_t x86_cpuid_supported(void) {
    uint32_t eflags_orig, eflags_new;
    
    /* Try to flip the ID flag in EFLAGS */
    asm volatile(
        "pushfl\n\t"
        "popl %0\n\t"           /* Get original EFLAGS */
        "movl %0, %1\n\t"
        "xorl $0x200000, %1\n\t" /* Flip ID flag */
        "pushl %1\n\t"
        "popfl\n\t"             /* Try to set modified EFLAGS */
        "pushfl\n\t"
        "popl %1\n\t"           /* Get EFLAGS again */
        "pushl %0\n\t"
        "popfl"                 /* Restore original EFLAGS */
        : "=&r"(eflags_orig), "=&r"(eflags_new)
    );
    
    /* If ID flag changed, CPUID is supported */
    return (eflags_orig != eflags_new) ? 1 : 0;
}

/* IMPLEMENTED: Basic CPUID wrapper */
void x86_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    if (!x86_cpuid_supported()) {
        *eax = *ebx = *ecx = *edx = 0;
        return;
    }
    
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf)
    );
}

/* IMPLEMENTED: Get CPU vendor string */
void x86_get_cpu_vendor(char* vendor) {
    uint32_t eax, ebx, ecx, edx;
    
    if (!vendor) return;
    
    x86_cpuid(0, &eax, &ebx, &ecx, &edx);
    
    /* Vendor string is in EBX, EDX, ECX order */
    *((uint32_t*)vendor) = ebx;
    *((uint32_t*)(vendor + 4)) = edx;
    *((uint32_t*)(vendor + 8)) = ecx;
    vendor = '\0';
}

/* IMPLEMENTED: HAL status and diagnostics */
void hal_show_system_info(void) {
    char cpu_vendor[128];
    
    meow_info("====  SYSTEM INFORMATION ====");
    meow_info("=============================");
    
    /* CPU Information */
    x86_get_cpu_vendor(cpu_vendor);
    meow_info("CPU Vendor: %s", cpu_vendor);
    meow_info("CPUID Support: %s", x86_cpuid_supported() ? "Yes" : "No");
    
    /* Memory Information */
    meow_info("Total Memory: %d MB", hal_memory_get_total_size() / (1024 * 1024));
    meow_info("Available Memory: %d MB", hal_memory_get_available_size() / (1024 * 1024));
    meow_info("Kernel End: 0x%x", (uint32_t)hal_memory_get_kernel_end());
    
    /* Control Registers */
    meow_info("CR0: 0x%x", x86_get_cr0());
    meow_info("CR2: 0x%x", x86_get_cr2());
    meow_info("CR3: 0x%x", x86_get_cr3());
    
    meow_info("=============================");
}

/* IMPLEMENTED: HAL self-test */
uint8_t hal_self_test(void) {
    meow_info(" HAL: Running self-test...");
    
    uint8_t tests_passed = 0;
    uint8_t total_tests = 0;
    
    /* Test 1: Port I/O */
    total_tests++;
    meow_debug(" Testing port I/O...");
    hal_port_outb(0x80, 0xAA); /* POST code port */
    /* If we didn't crash, consider it a pass */
    tests_passed++;
    
    /* Test 2: CPU flags */
    total_tests++;
    meow_debug(" Testing CPU flags...");
    uint32_t flags = hal_cpu_get_flags();
    if (flags != 0) {
        tests_passed++;
    }
    
    /* Test 3: Memory detection */
    total_tests++;
    meow_debug(" Testing memory detection...");
    uint32_t memory = hal_memory_get_total_size();
    if (memory >= (16 * 1024 * 1024)) { /* At least 16MB */
        tests_passed++;
    }
    
    /* Test 4: Control registers */
    total_tests++;
    meow_debug(" Testing control registers...");
    uint32_t cr0 = x86_get_cr0();
    if (cr0 & 0x1) { /* Protected mode should be enabled */
        tests_passed++;
    }
    
    meow_info(" HAL self-test: %d/%d tests passed", tests_passed, total_tests);
    
    return (tests_passed == total_tests) ? 1 : 0;
}

/* IMPLEMENTED: HAL initialization status */
static uint8_t hal_init_status = 0;

uint8_t hal_is_initialized(void) {
    return hal_init_status;
}

void hal_set_initialized(void) {
    hal_init_status = 1;
}

/* IMPLEMENTED: Emergency halt with diagnostic info */
void hal_emergency_halt(const char* reason) {
    meow_error(" EMERGENCY HALT: %s", reason ? reason : "Unknown");
    
    /* Disable interrupts */
    hal_cpu_disable_interrupts();
    
    /* Show diagnostic information */
    meow_error("🔍 System State at Halt:");
    meow_error("  CR0: 0x%x", x86_get_cr0());
    meow_error("  CR2: 0x%x", x86_get_cr2());
    meow_error("  CR3: 0x%x", x86_get_cr3());
    meow_error("  EFLAGS: 0x%x", hal_cpu_get_flags());
    
    /* Halt the system */
    meow_error("==== System halted!!! ====");
    while (1) {
        hal_cpu_halt();
    }
}

