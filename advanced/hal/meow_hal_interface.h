/* advanced/hal/meow_hal_interface.h - Hardware Abstraction Layer Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_HAL_INTERFACE_H
#define MEOW_HAL_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include "../kernel/meow_error_definitions.h"
#include "../kernel/meow_multiboot.h"           /* Multiboot info structure */

/* ============================================================================
 * HAL CONSTANTS AND CONFIGURATION
 * ============================================================================ */

/* HAL constants */
#define MEOW_HAL_MAX_DEBUG_STRING_LEN   256
#define MEOW_HAL_MAX_IRQ_HANDLERS       256
#define MEOW_HAL_INVALID_IRQ            0xFF

/* Architecture types */
typedef enum {
    MEOW_ARCH_UNKNOWN = 0,
    MEOW_ARCH_X86,
    MEOW_ARCH_X86_64,
    MEOW_ARCH_ARM64,
    MEOW_ARCH_RISCV64
} meow_arch_t;

/* ============================================================================
 * HAL FUNCTION POINTER STRUCTURES (Architecture Abstraction)
 * ============================================================================ */

/* Forward declarations */
struct hal_cpu_ops;
struct hal_memory_ops;
struct hal_interrupt_ops;
struct hal_timer_ops;
struct hal_io_ops;
struct hal_debug_ops;

/**
 * hal_cpu_ops - CPU operations function pointers
 * 
 * Architecture-specific CPU operations that must be implemented
 * by each supported architecture (x86, ARM64, etc.)
 */
struct hal_cpu_ops {
    /* Core CPU operations */
    meow_error_t (*init)(void);
    meow_error_t (*shutdown)(void);
    meow_error_t (*halt)(void);
    meow_error_t (*reset)(void);
    
    /* Interrupt control */
    meow_error_t (*disable_interrupts)(void);
    meow_error_t (*enable_interrupts)(void);
    uint32_t (*get_interrupt_flags)(void);
    meow_error_t (*set_interrupt_flags)(uint32_t flags);
    
    /* CPU feature detection */
    uint32_t (*get_cpu_features)(void);
    const char* (*get_cpu_vendor)(void);
    uint32_t (*get_cpu_frequency)(void);
    
    /* Power management */
    meow_error_t (*enter_sleep)(uint8_t sleep_level);
    meow_error_t (*exit_sleep)(void);
};

/**
 * hal_memory_ops - Memory management operations
 */
struct hal_memory_ops {
    /* Memory detection and initialization */
    meow_error_t (*init)(multiboot_info_t* mbi);
    uint32_t (*get_total_size)(void);
    uint32_t (*get_available_size)(void);
    void* (*get_kernel_end)(void);
    
    /* Memory mapping and protection */
    meow_error_t (*map_page)(void* virtual_addr, void* physical_addr, uint32_t flags);
    meow_error_t (*unmap_page)(void* virtual_addr);
    meow_error_t (*set_page_flags)(void* virtual_addr, uint32_t flags);
    
    /* Memory validation */
    meow_error_t (*validate_pointer)(const void* ptr);
    meow_error_t (*validate_range)(const void* start, size_t size);
    
    /* Cache operations */
    meow_error_t (*flush_cache)(void* addr, size_t size);
    meow_error_t (*invalidate_cache)(void* addr, size_t size);
};

/**
 * hal_interrupt_ops - Interrupt management operations
 */
struct hal_interrupt_ops {
    /* Interrupt system initialization */
    meow_error_t (*init)(void);
    meow_error_t (*shutdown)(void);
    
    /* Interrupt handler management */
    meow_error_t (*register_handler)(unsigned int irq, void (*handler)(uint8_t irq));
    meow_error_t (*unregister_handler)(unsigned int irq);
    void (*handler_stub)(unsigned int irq);
    
    /* Interrupt control */
    meow_error_t (*enable_irq)(uint8_t irq);
    meow_error_t (*disable_irq)(uint8_t irq);
    meow_error_t (*ack_irq)(uint8_t irq);
    
    /* Interrupt information */
    uint8_t (*get_current_irq)(void);
    uint32_t (*get_irq_count)(uint8_t irq);
};

/**
 * hal_timer_ops - Timer operations
 */
struct hal_timer_ops {
    /* Timer initialization */
    meow_error_t (*init)(uint32_t frequency);
    meow_error_t (*shutdown)(void);
    
    /* Timer control */
    meow_error_t (*start)(void);
    meow_error_t (*stop)(void);
    meow_error_t (*set_frequency)(uint32_t frequency);
    uint32_t (*get_frequency)(void);
    
    /* Time measurement */
    uint64_t (*get_ticks)(void);
    uint64_t (*get_milliseconds)(void);
    meow_error_t (*sleep)(uint32_t milliseconds);
    
    /* Timer callbacks */
    meow_error_t (*register_callback)(void (*callback)(void));
    meow_error_t (*unregister_callback)(void);
};

/**
 * hal_io_ops - I/O operations (mainly for x86 port I/O)
 */
struct hal_io_ops {
    /* Port I/O (x86 specific, stubs for other architectures) */
    uint8_t (*inb)(uint16_t port);
    uint16_t (*inw)(uint16_t port);
    uint32_t (*inl)(uint16_t port);
    meow_error_t (*outb)(uint16_t port, uint8_t data);
    meow_error_t (*outw)(uint16_t port, uint16_t data);
    meow_error_t (*outl)(uint16_t port, uint32_t data);
    
    /* Memory-mapped I/O */
    uint8_t (*read8)(void* addr);
    uint16_t (*read16)(void* addr);
    uint32_t (*read32)(void* addr);
    uint64_t (*read64)(void* addr);
    meow_error_t (*write8)(void* addr, uint8_t data);
    meow_error_t (*write16)(void* addr, uint16_t data);
    meow_error_t (*write32)(void* addr, uint32_t data);
    meow_error_t (*write64)(void* addr, uint64_t data);
};

/**
 * hal_debug_ops - Debug and diagnostic operations
 */
struct hal_debug_ops {
    /* Debug output */
    meow_error_t (*init)(void);
    meow_error_t (*putc)(char c);
    meow_error_t (*puts)(const char* str);
    meow_error_t (*printf)(const char* format, ...);
    
    /* System information */
    meow_error_t (*show_system_info)(void);
    meow_error_t (*dump_registers)(void);
    meow_error_t (*dump_stack)(void* stack_ptr, size_t size);
    
    /* Self-test operations */
    meow_error_t (*self_test)(void);
    uint8_t (*get_test_results)(void);
};

/* ============================================================================
 * MAIN HAL OPERATIONS STRUCTURE
 * ============================================================================ */

/**
 * hal_ops - Main HAL operations structure
 * 
 * This structure contains pointers to all architecture-specific
 * operation structures. Each architecture must provide implementations
 * for all these operations.
 */
typedef struct hal_ops {
    /* Architecture identification */
    meow_arch_t architecture;
    const char* arch_name;
    uint32_t arch_version;
    
    /* HAL operation structures */
    const struct hal_cpu_ops* cpu_ops;
    const struct hal_memory_ops* memory_ops;
    const struct hal_interrupt_ops* interrupt_ops;
    const struct hal_timer_ops* timer_ops;
    const struct hal_io_ops* io_ops;
    const struct hal_debug_ops* debug_ops;
    
    /* HAL management */
    meow_error_t (*init)(multiboot_info_t* mbi);
    meow_error_t (*shutdown)(void);
    meow_error_t (*self_test)(void);
    uint8_t (*is_initialized)(void);
    
    /* Emergency operations */
    void (*emergency_halt)(const char* reason);
    void (*panic)(const char* message);
} hal_ops_t;

/* ============================================================================
 * GLOBAL HAL INTERFACE FUNCTIONS
 * ============================================================================ */

/* HAL initialization and management */
meow_error_t hal_init(multiboot_info_t* mbi);
meow_error_t hal_shutdown(void);
meow_error_t hal_register_ops(const hal_ops_t* ops);
const hal_ops_t* hal_get_ops(void);
uint8_t hal_is_initialized(void);

/* Architecture registration functions */
meow_error_t hal_register_x86_ops(void);
meow_error_t hal_register_arm64_ops(void);

/* Convenience macros for accessing HAL operations */
#define HAL_CPU_OP(op, ...)        (hal_get_ops()->cpu_ops->op(__VA_ARGS__))
#define HAL_MEMORY_OP(op, ...)     (hal_get_ops()->memory_ops->op(__VA_ARGS__))
#define HAL_INTERRUPT_OP(op, ...)  (hal_get_ops()->interrupt_ops->op(__VA_ARGS__))
#define HAL_TIMER_OP(op, ...)      (hal_get_ops()->timer_ops->op(__VA_ARGS__))
#define HAL_IO_OP(op, ...)         (hal_get_ops()->io_ops->op(__VA_ARGS__))
#define HAL_DEBUG_OP(op, ...)      (hal_get_ops()->debug_ops->op(__VA_ARGS__))

/* Convenience wrapper functions (for backward compatibility) */
static inline meow_error_t hal_cpu_init(void) {
    return HAL_CPU_OP(init);
}

static inline meow_error_t hal_cpu_halt(void) {
    return HAL_CPU_OP(halt);
}

static inline meow_error_t hal_cpu_disable_interrupts(void) {
    return HAL_CPU_OP(disable_interrupts);
}

static inline meow_error_t hal_cpu_enable_interrupts(void) {
    return HAL_CPU_OP(enable_interrupts);
}

static inline uint32_t hal_memory_get_total_size(void) {
    return HAL_MEMORY_OP(get_total_size);
}

static inline meow_error_t hal_interrupt_enable_irq(uint8_t irq) {
    return HAL_INTERRUPT_OP(enable_irq, irq);
}

static inline uint8_t hal_io_inb(uint16_t port) {
    return HAL_IO_OP(inb, port);
}

static inline meow_error_t hal_io_outb(uint16_t port, uint8_t data) {
    return HAL_IO_OP(outb, port, data);
}

/* ============================================================================
 * HAL EMERGENCY AND MANAGEMENT FUNCTIONS
 * ============================================================================ */

/**
 * hal_emergency_halt - Emergency system halt
 * @reason: Reason for the emergency halt
 */
void hal_emergency_halt(const char* reason);

/**
 * hal_panic - System panic with message
 * @message: Panic message
 */
void hal_panic(const char* message);

/**
 * hal_get_architecture_info - Get detailed architecture information
 * @arch: Output pointer for architecture type
 * @name: Output pointer for architecture name
 * @version: Output pointer for architecture version
 */
meow_error_t hal_get_architecture_info(meow_arch_t* arch, const char** name, uint32_t* version);

/**
 * hal_print_system_info - Print comprehensive system information
 */
void hal_print_system_info(void);

/**
 * hal_validate_ops_structure - Validate HAL operations structure
 * @ops: Operations structure to validate
 */
meow_error_t hal_validate_ops_structure(const hal_ops_t* ops);

/* ============================================================================
 * UTILITY MACROS AND INLINE FUNCTIONS
 * ============================================================================ */

/* Architecture detection */
static inline meow_arch_t hal_get_architecture(void) {
    const hal_ops_t* ops = hal_get_ops();
    return ops ? ops->architecture : MEOW_ARCH_UNKNOWN;
}

static inline const char* hal_get_arch_name(void) {
    const hal_ops_t* ops = hal_get_ops();
    return ops ? ops->arch_name : "unknown";
}

/* HAL operation validation macros */
#define HAL_VALIDATE_OP(ops_struct, op_name) \
    ((ops_struct) && (ops_struct)->op_name)

#define HAL_CALL_OP_SAFE(ops_struct, op_name, default_ret, ...) \
    (HAL_VALIDATE_OP(ops_struct, op_name) ? \
     (ops_struct)->op_name(__VA_ARGS__) : (default_ret))

/* Safe HAL operation macros that check for null pointers */
#define HAL_CPU_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->cpu_ops : NULL, op, default_ret, ##__VA_ARGS__)

#define HAL_MEMORY_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->memory_ops : NULL, op, default_ret, ##__VA_ARGS__)

#define HAL_INTERRUPT_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->interrupt_ops : NULL, op, default_ret, ##__VA_ARGS__)

#define HAL_TIMER_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->timer_ops : NULL, op, default_ret, ##__VA_ARGS__)

#define HAL_IO_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->io_ops : NULL, op, default_ret, ##__VA_ARGS__)

#define HAL_DEBUG_OP_SAFE(op, default_ret, ...) \
    HAL_CALL_OP_SAFE(hal_get_ops() ? hal_get_ops()->debug_ops : NULL, op, default_ret, ##__VA_ARGS__)

#endif /* MEOW_HAL_INTERFACE_H */