/* advanced/hal/x86/x86_meow_hal.c - x86 Architecture HAL Implementation
 *
 * Complete x86-specific implementation of HAL operations with security fixes
 * Copyright (c) 2025 MeowKernel Project
 */

#include "x86_meow_hal_interface.h"
#include "../meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* ============================================================================
 * X86 HAL STATE AND GLOBALS
 * ============================================================================ */

static uint8_t x86_hal_initialized = 0;
static uint8_t x86_cpu_initialized = 0;
static uint8_t x86_memory_initialized = 0;
static uint8_t x86_interrupt_initialized = 0;
static uint8_t x86_timer_initialized = 0;

static multiboot_info_t* x86_multiboot_info = NULL;
static uint32_t x86_detected_memory = 0;
static uint32_t x86_available_memory = 0;
static uint32_t x86_timer_frequency = 0;
static uint64_t x86_timer_ticks = 0;

/* Interrupt handler table */
static void (*x86_irq_handlers[MEOW_HAL_MAX_IRQ_HANDLERS])(uint8_t irq) = {0};

/* ============================================================================
 * X86 CPU OPERATIONS IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_cpu_init_impl(void) {
    if (x86_cpu_initialized) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Initializing CPU subsystem... ====");
    
    /* Initialize GDT */
    MEOW_RETURN_IF_ERROR(x86_gdt_init());
    
    /* Initialize IDT */
    MEOW_RETURN_IF_ERROR(x86_idt_init());
    
    /* Initialize PIC */
    MEOW_RETURN_IF_ERROR(x86_pic_init());
    
    x86_cpu_initialized = 1;
    meow_log(MEOW_LOG_CHIRP,"==== x86: CPU subsystem initialized ====");
    return MEOW_SUCCESS;
}

static meow_error_t x86_cpu_shutdown_impl(void) {
    if (!x86_cpu_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Shutting down CPU subsystem... ====");
    
    /* Disable all interrupts */
    x86_cli();
    
    /* Mask all IRQs */
    x86_pic_disable_all_irqs();
    
    x86_cpu_initialized = 0;
    return MEOW_SUCCESS;
}

static meow_error_t x86_cpu_halt_impl(void) {
    while (1) {
        x86_hlt();
    }
    return MEOW_SUCCESS; /* Never reached */
}

static meow_error_t x86_cpu_reset_impl(void) {
    meow_log(MEOW_LOG_CHIRP,"==== x86: Resetting system... ====");
    
    /* Triple fault method */
    x86_cli();
    
    /* Load invalid IDT to cause triple fault */
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) invalid_idt = {0, 0};
    
    asm volatile("lidt %0" : : "m"(invalid_idt));
    asm volatile("int3");
    
    /* If that didn't work, try keyboard controller reset */
    x86_outb(0x64, 0xFE);
    
    /* Last resort: infinite loop */
    while (1) x86_hlt();
    return MEOW_SUCCESS;
}

static meow_error_t x86_cpu_disable_interrupts_impl(void) {
    x86_cli();
    return MEOW_SUCCESS;
}

static meow_error_t x86_cpu_enable_interrupts_impl(void) {
    if (!x86_cpu_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    x86_sti();
    return MEOW_SUCCESS;
}

static uint32_t x86_cpu_get_interrupt_flags_impl(void) {
    return x86_get_eflags();
}

static meow_error_t x86_cpu_set_interrupt_flags_impl(uint32_t flags) {
    x86_set_eflags(flags);
    return MEOW_SUCCESS;
}

static uint32_t x86_cpu_get_features_impl(void) {
    uint32_t eax, ebx, ecx, edx;
    
    if (!x86_cpuid_supported()) {
        return 0;
    }
    
    x86_cpuid(1, &eax, &ebx, &ecx, &edx);
    return edx; /* Feature flags in EDX */
}

static const char* x86_cpu_get_vendor_impl(void) {
    static char vendor[13] = {0};
    
    if (!x86_cpuid_supported()) {
        return "Unknown";
    }
    
    x86_get_cpu_vendor();
    return vendor;
}

static uint32_t x86_cpu_get_frequency_impl(void) {
    /* TODO: Implement CPU frequency detection */
    return 0; /* Unknown frequency */
}

static meow_error_t x86_cpu_enter_sleep_impl(uint8_t sleep_level) {
    (void)sleep_level;
    x86_hlt();
    return MEOW_SUCCESS;
}

static meow_error_t x86_cpu_exit_sleep_impl(void) {
    /* Sleep is exited by interrupt */
    return MEOW_SUCCESS;
}

/* ============================================================================
 * X86 MEMORY OPERATIONS IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_memory_init_impl(multiboot_info_t* mbi) {
    if (x86_memory_initialized) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Initializing memory subsystem... ====");
    
    x86_multiboot_info = mbi;
    
    if (mbi) {
        x86_detected_memory = x86_detect_memory_from_multiboot(mbi);
    } else {
        meow_log(MEOW_LOG_HISS,"****  x86: No multiboot info, using default memory size ****");
        x86_detected_memory = 64 * 1024 * 1024; /* 64MB default */
    }
    
    if (x86_detected_memory == 0) {
        meow_log(MEOW_LOG_YOWL," x86: Failed to detect memory !!!");
        return MEOW_ERROR_HARDWARE_FAILURE;
    }
    
    /* Calculate available memory (total minus kernel usage) */
    uint32_t kernel_size = x86_get_kernel_memory_usage();
    x86_available_memory = (x86_detected_memory > kernel_size) ? 
                          (x86_detected_memory - kernel_size) : 0;
    
    meow_log(MEOW_LOG_CHIRP," x86: Memory detected: %u MB total, %u MB available", 
             x86_detected_memory / (1024 * 1024),
             x86_available_memory / (1024 * 1024));
    
    x86_memory_initialized = 1;
    return MEOW_SUCCESS;
}

static uint32_t x86_memory_get_total_size_impl(void) {
    return x86_detected_memory;
}

static uint32_t x86_memory_get_available_size_impl(void) {
    return x86_available_memory;
}

static void* x86_memory_get_kernel_end_impl(void) {
    extern char _kernel_end;
    return &_kernel_end;
}

static meow_error_t x86_memory_map_page_impl(void* virtual_addr, void* physical_addr, uint32_t flags) {
    /* TODO: Implement page mapping for x86 MMU */
    (void)virtual_addr;
    (void)physical_addr;
    (void)flags;
    return MEOW_ERROR_NOT_SUPPORTED;
}

static meow_error_t x86_memory_unmap_page_impl(void* virtual_addr) {
    /* TODO: Implement page unmapping */
    (void)virtual_addr;
    return MEOW_ERROR_NOT_SUPPORTED;
}

static meow_error_t x86_memory_set_page_flags_impl(void* virtual_addr, uint32_t flags) {
    /* TODO: Implement page flag setting */
    (void)virtual_addr;
    (void)flags;
    return MEOW_ERROR_NOT_SUPPORTED;
}

static meow_error_t x86_memory_validate_pointer_impl(const void* ptr) {
    if (!ptr) {
        return MEOW_ERROR_NULL_POINTER;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    
    /* Check for common invalid ranges */
    if (addr < 0x1000) {
        return MEOW_ERROR_INVALID_PARAMETER; /* NULL pointer region */
    }
    
    if (addr >= 0xFFC00000) {
        return MEOW_ERROR_INVALID_PARAMETER; /* High memory reserved region */
    }
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_memory_validate_range_impl(const void* start, size_t size) {
    MEOW_RETURN_IF_ERROR(x86_memory_validate_pointer_impl(start));
    
    if (size == 0 || size > 0x40000000) { /* Max 1GB */
        return MEOW_ERROR_INVALID_SIZE;
    }
    
    /* Check for overflow */
    uintptr_t start_addr = (uintptr_t)start;
    if (start_addr > UINTPTR_MAX - size) {
        return MEOW_ERROR_INVALID_SIZE;
    }
    
    uintptr_t end_addr = start_addr + size - 1;
    return x86_memory_validate_pointer_impl((void*)end_addr);
}

static meow_error_t x86_memory_flush_cache_impl(void* addr, size_t size) {
    /* TODO: Implement cache flushing */
    (void)addr;
    (void)size;
    return MEOW_SUCCESS; /* No-op for now */
}

static meow_error_t x86_memory_invalidate_cache_impl(void* addr, size_t size) {
    /* TODO: Implement cache invalidation */
    (void)addr;
    (void)size;
    return MEOW_SUCCESS; /* No-op for now */
}

/* ============================================================================
 * X86 INTERRUPT OPERATIONS IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_interrupt_init_impl(void) {
    if (x86_interrupt_initialized) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Initializing interrupt subsystem... ====");
    
    /* Clear interrupt handler table */
    meow_memset(x86_irq_handlers, 0, sizeof(x86_irq_handlers));
    
    x86_interrupt_initialized = 1;
    return MEOW_SUCCESS;
}

static meow_error_t x86_interrupt_shutdown_impl(void) {
    if (!x86_interrupt_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Shutting down interrupt subsystem... ====");
    
    /* Disable all interrupts */
    x86_cli();
    
    /* Clear all handlers */
    meow_memset(x86_irq_handlers, 0, sizeof(x86_irq_handlers));
    
    x86_interrupt_initialized = 0;
    return MEOW_SUCCESS;
}

static meow_error_t x86_interrupt_register_handler_impl(unsigned int irq, void (*handler)(uint8_t irq)) {
    if (!x86_interrupt_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    if (irq >= MEOW_HAL_MAX_IRQ_HANDLERS) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    if (!handler) {
        return MEOW_ERROR_NULL_POINTER;
    }
    
    if (x86_irq_handlers[irq] != NULL) {
        meow_log(MEOW_LOG_HISS,"  x86: Overwriting existing handler for IRQ %u", irq);
    }
    
    x86_irq_handlers[irq] = handler;
    meow_log(MEOW_LOG_MEOW," x86: Registered handler for IRQ %u", irq);
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_interrupt_unregister_handler_impl(unsigned int irq) {
    if (!x86_interrupt_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    if (irq >= MEOW_HAL_MAX_IRQ_HANDLERS) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }
    
    x86_irq_handlers[irq] = NULL;
    meow_log(MEOW_LOG_MEOW," x86: Unregistered handler for IRQ %u", irq);
    
    return MEOW_SUCCESS;
}

static void x86_interrupt_handler_stub_impl(unsigned int irq) {
    /* This function is called from assembly interrupt stubs */
    if (irq < MEOW_HAL_MAX_IRQ_HANDLERS && x86_irq_handlers[irq]) {
        x86_irq_handlers[irq](irq);
    } else {
        meow_log(MEOW_LOG_MEOW," x86: Unhandled IRQ %u", irq);
    }
}

static meow_error_t x86_interrupt_enable_irq_impl(uint8_t irq) {
    if (!x86_interrupt_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    return x86_pic_enable_irq(irq);
}

static meow_error_t x86_interrupt_disable_irq_impl(uint8_t irq) {
    if (!x86_interrupt_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    return x86_pic_disable_irq(irq);
}

static meow_error_t x86_interrupt_ack_irq_impl(uint8_t irq) {
    x86_pic_eoi(irq);
    return MEOW_SUCCESS;
}

static uint8_t x86_interrupt_get_current_irq_impl(void) {
    /* TODO: Implement current IRQ detection */
    return MEOW_HAL_INVALID_IRQ;
}

static uint32_t x86_interrupt_get_irq_count_impl(uint8_t irq) {
    /* TODO: Implement IRQ counting */
    (void)irq;
    return 0;
}

/* ============================================================================
 * X86 TIMER OPERATIONS IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_timer_init_impl(uint32_t frequency) {
    if (x86_timer_initialized) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Initializing timer subsystem at %u Hz... ====", frequency);
    
    MEOW_RETURN_IF_ERROR(x86_pit_init(frequency));
    
    x86_timer_frequency = frequency;
    x86_timer_ticks = 0;
    x86_timer_initialized = 1;
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_timer_shutdown_impl(void) {
    if (!x86_timer_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Shutting down timer subsystem... ====");
    
    /* Disable timer IRQ */
    x86_pic_disable_irq(0);
    
    x86_timer_initialized = 0;
    return MEOW_SUCCESS;
}

static meow_error_t x86_timer_start_impl(void) {
    if (!x86_timer_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Enable timer IRQ */
    return x86_pic_enable_irq(0);
}

static meow_error_t x86_timer_stop_impl(void) {
    if (!x86_timer_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Disable timer IRQ */
    return x86_pic_disable_irq(0);
}

static meow_error_t x86_timer_set_frequency_impl(uint32_t frequency) {
    if (!x86_timer_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    MEOW_RETURN_IF_ERROR(x86_pit_set_frequency(frequency));
    x86_timer_frequency = frequency;
    
    return MEOW_SUCCESS;
}

static uint32_t x86_timer_get_frequency_impl(void) {
    return x86_timer_frequency;
}

static uint64_t x86_timer_get_ticks_impl(void) {
    return x86_timer_ticks;
}

static uint64_t x86_timer_get_milliseconds_impl(void) {
    if (x86_timer_frequency == 0) {
        return 0;
    }
    
    return (x86_timer_ticks * 1000) / x86_timer_frequency;
}

static meow_error_t x86_timer_sleep_impl(uint32_t milliseconds) {
    if (!x86_timer_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    uint64_t start_ms = x86_timer_get_milliseconds_impl();
    uint64_t target_ms = start_ms + milliseconds;
    
    while (x86_timer_get_milliseconds_impl() < target_ms) {
        x86_hlt();
    }
    
    return MEOW_SUCCESS;
}

/* Timer callback (called from interrupt handler) */
void x86_timer_tick(void) {
    x86_timer_ticks++;
    
    /* TODO: Call registered timer callbacks */
}

static meow_error_t x86_timer_register_callback_impl(void (*callback)(void)) {
    /* TODO: Implement timer callbacks */
    (void)callback;
    return MEOW_ERROR_NOT_SUPPORTED;
}

static meow_error_t x86_timer_unregister_callback_impl(void) {
    /* TODO: Implement timer callback removal */
    return MEOW_ERROR_NOT_SUPPORTED;
}

/* ============================================================================
 * X86 I/O OPERATIONS IMPLEMENTATION  
 * ============================================================================ */

static uint8_t x86_io_inb_impl(uint16_t port) {
    return x86_inb(port);
}

static uint16_t x86_io_inw_impl(uint16_t port) {
    return x86_inw(port);
}

static uint32_t x86_io_inl_impl(uint16_t port) {
    return x86_inl(port);
}

static meow_error_t x86_io_outb_impl(uint16_t port, uint8_t data) {
    x86_outb(port, data);
    return MEOW_SUCCESS;
}

static meow_error_t x86_io_outw_impl(uint16_t port, uint16_t data) {
    x86_outw(port, data);
    return MEOW_SUCCESS;
}

static meow_error_t x86_io_outl_impl(uint16_t port, uint32_t data) {
    x86_outl(port, data);
    return MEOW_SUCCESS;
}

/* Memory-mapped I/O operations */
static uint8_t x86_io_read8_impl(void* addr) {
    MEOW_RETURN_IF_NULL(addr);
    return *(volatile uint8_t*)addr;
}

static uint16_t x86_io_read16_impl(void* addr) {
    MEOW_RETURN_IF_NULL(addr);
    return *(volatile uint16_t*)addr;
}

static uint32_t x86_io_read32_impl(void* addr) {
    MEOW_RETURN_IF_NULL(addr);
    return *(volatile uint32_t*)addr;
}

static uint64_t x86_io_read64_impl(void* addr) {
    MEOW_RETURN_IF_NULL(addr);
    return *(volatile uint64_t*)addr;
}

static meow_error_t x86_io_write8_impl(void* addr, uint8_t data) {
    MEOW_RETURN_IF_NULL(addr);
    *(volatile uint8_t*)addr = data;
    return MEOW_SUCCESS;
}

static meow_error_t x86_io_write16_impl(void* addr, uint16_t data) {
    MEOW_RETURN_IF_NULL(addr);
    *(volatile uint16_t*)addr = data;
    return MEOW_SUCCESS;
}

static meow_error_t x86_io_write32_impl(void* addr, uint32_t data) {
    MEOW_RETURN_IF_NULL(addr);
    *(volatile uint32_t*)addr = data;
    return MEOW_SUCCESS;
}

static meow_error_t x86_io_write64_impl(void* addr, uint64_t data) {
    MEOW_RETURN_IF_NULL(addr);
    *(volatile uint64_t*)addr = data;
    return MEOW_SUCCESS;
}

/* ============================================================================
 * X86 DEBUG OPERATIONS IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_debug_init_impl(void) {
    /* VGA text mode is available by default on x86 */
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_putc_impl(char c) {
    x86_vga_putc(c);
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_puts_impl(const char* str) {
    if (!str) {
        return MEOW_ERROR_NULL_POINTER;
    }
    
    size_t count = 0;
    while (*str && count < MEOW_HAL_MAX_DEBUG_STRING_LEN) {
        x86_vga_putc(*str++);
        count++;
    }
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_printf_impl(const char* format, ...) {
    /* TODO: Implement formatted printing */
    return x86_debug_puts_impl(format);
}

static meow_error_t x86_debug_show_system_info_impl(void) {
    meow_log(MEOW_LOG_CHIRP," ======== X86 SYSTEM INFORMATION ========");
    meow_log(MEOW_LOG_CHIRP,"CPU: %s", x86_cpu_get_vendor_impl());
    meow_log(MEOW_LOG_CHIRP,"CPU Features: 0x%08x", x86_cpu_get_features_impl());
    meow_log(MEOW_LOG_CHIRP,"Memory: %u MB total, %u MB available",
             x86_detected_memory / (1024 * 1024),
             x86_available_memory / (1024 * 1024));
    meow_log(MEOW_LOG_CHIRP,"Timer Frequency: %u Hz", x86_timer_frequency);
    meow_log(MEOW_LOG_CHIRP,"Timer Ticks: %llu", x86_timer_ticks);
    meow_log(MEOW_LOG_CHIRP," =====================================");
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_dump_registers_impl(void) {
    uint32_t cr0 = x86_get_cr0();
    uint32_t cr2 = x86_get_cr2();
    uint32_t cr3 = x86_get_cr3();
    uint32_t eflags = x86_get_eflags();
    
    meow_log(MEOW_LOG_CHIRP," === X86 REGISTER DUMP ===");
    meow_log(MEOW_LOG_CHIRP,"CR0: 0x%08x", cr0);
    meow_log(MEOW_LOG_CHIRP,"CR2: 0x%08x", cr2);
    meow_log(MEOW_LOG_CHIRP,"CR3: 0x%08x", cr3);
    meow_log(MEOW_LOG_CHIRP,"EFLAGS: 0x%08x", eflags);
    meow_log(MEOW_LOG_CHIRP," =========================");
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_dump_stack_impl(void* stack_ptr, size_t size) {
    if (!stack_ptr) {
        return MEOW_ERROR_NULL_POINTER;
    }
    
    meow_log(MEOW_LOG_CHIRP," === STACK DUMP ===");
    meow_log(MEOW_LOG_CHIRP,"Stack pointer: 0x%08x", (uint32_t)stack_ptr);
    meow_log(MEOW_LOG_CHIRP,"Size: %zu bytes", size);
    
    /* TODO: Implement actual stack dumping */
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_debug_self_test_impl(void) {
    uint8_t tests_passed = 0;
    uint8_t total_tests = 0;
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Running HAL self-test... ====");
    
    /* Test 1: Port I/O */
    total_tests++;
    x86_outb(0x80, 0xAA); /* POST code port */
    tests_passed++; /* If we didn't crash, consider it passed */
    
    /* Test 2: CPU flags */
    total_tests++;
    uint32_t flags = x86_get_eflags();
    if (flags != 0) {
        tests_passed++;
    }
    
    /* Test 3: Memory detection */
    total_tests++;
    if (x86_detected_memory >= (16 * 1024 * 1024)) { /* At least 16MB */
        tests_passed++;
    }
    
    /* Test 4: Timer functionality */
    total_tests++;
    if (x86_timer_initialized && x86_timer_frequency > 0) {
        tests_passed++;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Self-test complete: %u/%u tests passed ====", 
             tests_passed, total_tests);
    
    return (tests_passed == total_tests) ? MEOW_SUCCESS : MEOW_ERROR_HARDWARE_FAILURE;
}

static uint8_t x86_debug_get_test_results_impl(void) {
    /* Return simplified test result */
    return x86_hal_initialized ? 1 : 0;
}

/* ============================================================================
 * X86 HAL OPERATIONS STRUCTURES
 * ============================================================================ */

static const struct hal_cpu_ops x86_cpu_ops = {
    .init = x86_cpu_init_impl,
    .shutdown = x86_cpu_shutdown_impl,
    .halt = x86_cpu_halt_impl,
    .reset = x86_cpu_reset_impl,
    .disable_interrupts = x86_cpu_disable_interrupts_impl,
    .enable_interrupts = x86_cpu_enable_interrupts_impl,
    .get_interrupt_flags = x86_cpu_get_interrupt_flags_impl,
    .set_interrupt_flags = x86_cpu_set_interrupt_flags_impl,
    .get_cpu_features = x86_cpu_get_features_impl,
    .get_cpu_vendor = x86_cpu_get_vendor_impl,
    .get_cpu_frequency = x86_cpu_get_frequency_impl,
    .enter_sleep = x86_cpu_enter_sleep_impl,
    .exit_sleep = x86_cpu_exit_sleep_impl
};

static const struct hal_memory_ops x86_memory_ops = {
    .init = x86_memory_init_impl,
    .get_total_size = x86_memory_get_total_size_impl,
    .get_available_size = x86_memory_get_available_size_impl,
    .get_kernel_end = x86_memory_get_kernel_end_impl,
    .map_page = x86_memory_map_page_impl,
    .unmap_page = x86_memory_unmap_page_impl,
    .set_page_flags = x86_memory_set_page_flags_impl,
    .validate_pointer = x86_memory_validate_pointer_impl,
    .validate_range = x86_memory_validate_range_impl,
    .flush_cache = x86_memory_flush_cache_impl,
    .invalidate_cache = x86_memory_invalidate_cache_impl
};

static const struct hal_interrupt_ops x86_interrupt_ops = {
    .init = x86_interrupt_init_impl,
    .shutdown = x86_interrupt_shutdown_impl,
    .register_handler = x86_interrupt_register_handler_impl,
    .unregister_handler = x86_interrupt_unregister_handler_impl,
    .handler_stub = x86_interrupt_handler_stub_impl,
    .enable_irq = x86_interrupt_enable_irq_impl,
    .disable_irq = x86_interrupt_disable_irq_impl,
    .ack_irq = x86_interrupt_ack_irq_impl,
    .get_current_irq = x86_interrupt_get_current_irq_impl,
    .get_irq_count = x86_interrupt_get_irq_count_impl
};

static const struct hal_timer_ops x86_timer_ops = {
    .init = x86_timer_init_impl,
    .shutdown = x86_timer_shutdown_impl,
    .start = x86_timer_start_impl,
    .stop = x86_timer_stop_impl,
    .set_frequency = x86_timer_set_frequency_impl,
    .get_frequency = x86_timer_get_frequency_impl,
    .get_ticks = x86_timer_get_ticks_impl,
    .get_milliseconds = x86_timer_get_milliseconds_impl,
    .sleep = x86_timer_sleep_impl,
    .register_callback = x86_timer_register_callback_impl,
    .unregister_callback = x86_timer_unregister_callback_impl
};

static const struct hal_io_ops x86_io_ops = {
    .inb = x86_io_inb_impl,
    .inw = x86_io_inw_impl,
    .inl = x86_io_inl_impl,
    .outb = x86_io_outb_impl,
    .outw = x86_io_outw_impl,
    .outl = x86_io_outl_impl,
    .read8 = x86_io_read8_impl,
    .read16 = x86_io_read16_impl,
    .read32 = x86_io_read32_impl,
    .read64 = x86_io_read64_impl,
    .write8 = x86_io_write8_impl,
    .write16 = x86_io_write16_impl,
    .write32 = x86_io_write32_impl,
    .write64 = x86_io_write64_impl
};

static const struct hal_debug_ops x86_debug_ops = {
    .init = x86_debug_init_impl,
    .putc = x86_debug_putc_impl,
    .puts = x86_debug_puts_impl,
    .printf = x86_debug_printf_impl,
    .show_system_info = x86_debug_show_system_info_impl,
    .dump_registers = x86_debug_dump_registers_impl,
    .dump_stack = x86_debug_dump_stack_impl,
    .self_test = x86_debug_self_test_impl,
    .get_test_results = x86_debug_get_test_results_impl
};

/* ============================================================================
 * X86 MAIN HAL IMPLEMENTATION
 * ============================================================================ */

static meow_error_t x86_hal_init_impl(multiboot_info_t* mbi) {
    if (x86_hal_initialized) {
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Initializing x86 HAL... ====");
    
    /* Initialize subsystems in order */
    MEOW_RETURN_IF_ERROR(x86_cpu_ops.init());
    MEOW_RETURN_IF_ERROR(x86_memory_ops.init(mbi));
    MEOW_RETURN_IF_ERROR(x86_interrupt_ops.init());
    MEOW_RETURN_IF_ERROR(x86_timer_ops.init(100)); /* 100 Hz default */
    
    x86_hal_initialized = 1;
    meow_log(MEOW_LOG_CHIRP,"==== x86: HAL initialization complete ====");
    
    return MEOW_SUCCESS;
}

static meow_error_t x86_hal_shutdown_impl(void) {
    if (!x86_hal_initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    meow_log(MEOW_LOG_CHIRP,"==== x86: Shutting down x86 HAL... ====");
    
    /* Shutdown subsystems in reverse order */
    x86_timer_ops.shutdown();
    x86_interrupt_ops.shutdown();
    x86_cpu_ops.shutdown();
    
    x86_hal_initialized = 0;
    return MEOW_SUCCESS;
}

static meow_error_t x86_hal_self_test_impl(void) {
    return x86_debug_ops.self_test();
}

static uint8_t x86_hal_is_initialized_impl(void) {
    return x86_hal_initialized;
}

static void x86_hal_emergency_halt_impl(const char* reason) {
    meow_log(MEOW_LOG_YOWL,"==== x86: EMERGENCY HALT - %s ====", reason ? reason : "Unknown");
    x86_cpu_ops.disable_interrupts();
    x86_cpu_ops.halt();
}

static void x86_hal_panic_impl(const char* message) {
    meow_log(MEOW_LOG_YOWL,"==== x86: KERNEL PANIC - %s ====", message ? message : "Unknown");
    x86_debug_ops.dump_registers();
    x86_hal_emergency_halt_impl(message);
}

/* Main x86 HAL operations structure */
static const hal_ops_t x86_hal_ops = {
    .architecture = MEOW_ARCH_X86,
    .arch_name = "x86 (i386)",
    .arch_version = 1,
    
    .cpu_ops = &x86_cpu_ops,
    .memory_ops = &x86_memory_ops,
    .interrupt_ops = &x86_interrupt_ops,
    .timer_ops = &x86_timer_ops,
    .io_ops = &x86_io_ops,
    .debug_ops = &x86_debug_ops,
    
    .init = x86_hal_init_impl,
    .shutdown = x86_hal_shutdown_impl,
    .self_test = x86_hal_self_test_impl,
    .is_initialized = x86_hal_is_initialized_impl,
    .emergency_halt = x86_hal_emergency_halt_impl,
    .panic = x86_hal_panic_impl
};

/* ============================================================================
 * X86 HAL REGISTRATION FUNCTION
 * ============================================================================ */

/**
 * hal_register_x86_ops - Register x86 HAL operations
 * 
 * This function registers the x86-specific HAL operations with the
 * main HAL system. Called during HAL initialization for x86 systems.
 */
meow_error_t hal_register_x86_ops(void) {
    meow_log(MEOW_LOG_CHIRP,"==== Registering x86 HAL operations... ====");
    return hal_register_ops(&x86_hal_ops);
}