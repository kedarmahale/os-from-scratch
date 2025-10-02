/*
 * advanced/hal/meow_hal_manager.c - Hardware Abstraction Layer Implementation
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* ============================================================================
 * GLOBAL HAL STATE
 * ============================================================================ */

static const hal_ops_t* g_current_hal_ops = NULL;
static uint8_t g_hal_initialized = 0;
static meow_arch_t g_detected_arch = MEOW_ARCH_UNKNOWN;

/* ============================================================================
 * ARCHITECTURE DETECTION
 * ============================================================================ */

/**
 * detect_architecture - Detect the current architecture at runtime
 * 
 * This function uses compile-time and runtime detection to determine
 * which architecture we're running on.
 */
static meow_arch_t detect_architecture(void) {
    /* Use compile-time detection first */
#if defined(__i386__) || defined(__i386) || defined(i386)
    return MEOW_ARCH_X86;
#elif defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
    return MEOW_ARCH_X86_64;
#elif defined(__aarch64__)
    return MEOW_ARCH_ARM64;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return MEOW_ARCH_RISCV64;
#else
    /* Runtime detection could be added here */
    meow_log(MEOW_LOG_HISS, "Unknown architecture detected during compile-time check");
    return MEOW_ARCH_UNKNOWN;
#endif
}

/* ============================================================================
 * HAL OPERATIONS MANAGEMENT
 * ============================================================================ */

/**
 * hal_register_ops - Register HAL operations for the current architecture
 * @ops: Pointer to architecture-specific operations structure
 * 
 * This function registers the operations structure that contains all
 * the function pointers for the current architecture.
 */
meow_error_t hal_register_ops(const hal_ops_t* ops) {
    MEOW_RETURN_IF_NULL(ops);
    MEOW_RETURN_IF_NULL(ops->cpu_ops);
    MEOW_RETURN_IF_NULL(ops->memory_ops);
    MEOW_RETURN_IF_NULL(ops->interrupt_ops);
    MEOW_RETURN_IF_NULL(ops->timer_ops);
    MEOW_RETURN_IF_NULL(ops->io_ops);
    MEOW_RETURN_IF_NULL(ops->debug_ops);

    /* Validate architecture matches */
    if (g_detected_arch != MEOW_ARCH_UNKNOWN &&
        ops->architecture != g_detected_arch) {
        meow_log(MEOW_LOG_YOWL, "Architecture mismatch: detected %d, ops for %d",
                  g_detected_arch, ops->architecture);
        return MEOW_ERROR_INVALID_PARAMETER;
    }

    g_current_hal_ops = ops;
    g_detected_arch = ops->architecture;
    meow_log(MEOW_LOG_CHIRP, "HAL: Registered operations for %s", ops->arch_name);
    return MEOW_SUCCESS;
}

/**
 * hal_get_ops - Get current HAL operations structure
 * 
 * Returns the currently registered HAL operations structure.
 * This is used by the HAL_*_OP macros.
 */
const hal_ops_t* hal_get_ops(void) {
    return g_current_hal_ops;
}

/* ============================================================================
 * HAL INITIALIZATION AND MANAGEMENT
 * ============================================================================ */

/**
 * hal_init - Initialize the Hardware Abstraction Layer
 * @mbi: Multiboot information structure (may be NULL for non-multiboot systems)
 * 
 * This function detects the architecture, registers the appropriate
 * operations, and initializes all HAL subsystems.
 */
meow_error_t hal_init(multiboot_info_t* mbi) {
    meow_error_t result;

    if (g_hal_initialized) {
        meow_log(MEOW_LOG_MEOW, "HAL: Already initialized - cats are already in control");
        return MEOW_ERROR_ALREADY_INITIALIZED;
    }

    meow_log(MEOW_LOG_CHIRP, "==== HAL: Initializing Hardware Abstraction Layer... ====");

    /* Step 1: Detect architecture */
    g_detected_arch = detect_architecture();
    if (g_detected_arch == MEOW_ARCH_UNKNOWN) {
        meow_log(MEOW_LOG_SCREECH, "HAL: Unable to detect architecture - cats are confused!");
        return MEOW_ERROR_HARDWARE_FAILURE;
    }

    meow_log(MEOW_LOG_CHIRP, "HAL: Detected architecture: %d", g_detected_arch);

    /* Step 2: Register architecture-specific operations */
    switch (g_detected_arch) {
        case MEOW_ARCH_X86:
        case MEOW_ARCH_X86_64:
            result = hal_register_x86_ops();
            break;
        case MEOW_ARCH_ARM64:
            result = hal_register_arm64_ops();
            break;
        default:
            meow_log(MEOW_LOG_SCREECH, "HAL: Unsupported architecture: %d - cats don't know this hardware!", 
                     g_detected_arch);
            return MEOW_ERROR_NOT_SUPPORTED;
    }

    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "HAL: Failed to register operations: %d", result);
        return result;
    }

    /* Step 3: Initialize HAL subsystems */
    if (!g_current_hal_ops || !g_current_hal_ops->init) {
        meow_log(MEOW_LOG_SCREECH, "HAL: No operations registered or missing init function!");
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    result = g_current_hal_ops->init(mbi);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, "HAL: Architecture-specific initialization failed: %d", result);
        return result;
    }

    /* Step 4: Run self-test */
    if (g_current_hal_ops->self_test) {
        result = g_current_hal_ops->self_test();
        if (result != MEOW_SUCCESS) {
            meow_log(MEOW_LOG_HISS, "HAL: Self-test failed with code: %d (continuing anyway)", result);
        } else {
            meow_log(MEOW_LOG_CHIRP, "HAL: Self-test passed - cats are satisfied with hardware!");
        }
    }

    /* Step 5: Mark as initialized */
    g_hal_initialized = 1;
    meow_log(MEOW_LOG_CHIRP, "==== HAL: Initialization complete - cats now control hardware! ====");
    
    return MEOW_SUCCESS;
}

/**
 * hal_shutdown - Shutdown the Hardware Abstraction Layer
 */
meow_error_t hal_shutdown(void) {
    meow_error_t result = MEOW_SUCCESS;

    if (!g_hal_initialized) {
        meow_log(MEOW_LOG_MEOW, "HAL: Not initialized - nothing for cats to shut down");
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    meow_log(MEOW_LOG_CHIRP, "==== HAL: Shutting down Hardware Abstraction Layer... ====");

    /* Shutdown architecture-specific operations */
    if (g_current_hal_ops && g_current_hal_ops->shutdown) {
        result = g_current_hal_ops->shutdown();
        if (result != MEOW_SUCCESS) {
            meow_log(MEOW_LOG_HISS, "HAL: Architecture-specific shutdown failed: %d", result);
        }
    }

    /* Clear state */
    g_current_hal_ops = NULL;
    g_hal_initialized = 0;
    g_detected_arch = MEOW_ARCH_UNKNOWN;

    meow_log(MEOW_LOG_CHIRP, "==== HAL: Shutdown complete - cats have released hardware control ====");
    return result;
}

/**
 * hal_is_initialized - Check if HAL is initialized
 */
uint8_t hal_is_initialized(void) {
    return g_hal_initialized;
}

/* ============================================================================
 * HAL INFORMATION FUNCTIONS
 * ============================================================================ */

/**
 * hal_get_architecture_info - Get detailed architecture information
 */
meow_error_t hal_get_architecture_info(meow_arch_t* arch, const char** name, uint32_t* version) {
    if (!g_current_hal_ops) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    if (arch) *arch = g_current_hal_ops->architecture;
    if (name) *name = g_current_hal_ops->arch_name;
    if (version) *version = g_current_hal_ops->arch_version;

    return MEOW_SUCCESS;
}

/**
 * hal_print_system_info - Print comprehensive system information
 */
void hal_print_system_info(void) {
    if (!g_current_hal_ops) {
        meow_log(MEOW_LOG_YOWL, "HAL: Not initialized - no system info available");
        return;
    }

    meow_printf(" ==== HAL SYSTEM INFORMATION ====\n");
    meow_printf("Architecture: %s (ID: %d)\n", 
                g_current_hal_ops->arch_name, 
                g_current_hal_ops->architecture);
    meow_printf("Architecture Version: %u\n", g_current_hal_ops->arch_version);
    meow_printf("HAL Initialized: %s\n", g_hal_initialized ? "Yes" : "No");

    /* Show subsystem status */
    if (g_current_hal_ops->is_initialized) {
        meow_printf("HAL Subsystems: %s\n", 
                    g_current_hal_ops->is_initialized() ? "Active" : "Inactive");
    }

    meow_printf("===========================\n");
}

/**
 * hal_validate_ops_structure - Validate HAL operations structure
 */
meow_error_t hal_validate_ops_structure(const hal_ops_t* ops) {
    MEOW_RETURN_IF_NULL(ops);

    /* Check required operation structures */
    MEOW_RETURN_IF_NULL(ops->cpu_ops);
    MEOW_RETURN_IF_NULL(ops->memory_ops);
    MEOW_RETURN_IF_NULL(ops->interrupt_ops);
    MEOW_RETURN_IF_NULL(ops->timer_ops);
    MEOW_RETURN_IF_NULL(ops->io_ops);
    MEOW_RETURN_IF_NULL(ops->debug_ops);

    /* Check required management functions */
    MEOW_RETURN_IF_NULL(ops->init);
    MEOW_RETURN_IF_NULL(ops->shutdown);

    /* Check architecture info */
    if (ops->architecture == MEOW_ARCH_UNKNOWN) {
        return MEOW_ERROR_INVALID_PARAMETER;
    }

    MEOW_RETURN_IF_NULL(ops->arch_name);

    /* Validate critical CPU operations */
    MEOW_RETURN_IF_NULL(ops->cpu_ops->init);
    MEOW_RETURN_IF_NULL(ops->cpu_ops->halt);

    /* Validate critical memory operations */
    MEOW_RETURN_IF_NULL(ops->memory_ops->init);
    MEOW_RETURN_IF_NULL(ops->memory_ops->get_total_size);

    meow_log(MEOW_LOG_MEOW, "HAL: Operations structure validation passed - cats approve!");
    return MEOW_SUCCESS;
}

/* ============================================================================
 * HAL EMERGENCY FUNCTIONS
 * ============================================================================ */

/**
 * hal_emergency_halt - Emergency system halt
 */
void hal_emergency_halt(const char* reason) {
    meow_log(MEOW_LOG_SCREECH, "==== HAL: EMERGENCY HALT - %s ====", reason ? reason : "Unknown");

    /* Try to use architecture-specific emergency halt */
    if (g_current_hal_ops && g_current_hal_ops->emergency_halt) {
        g_current_hal_ops->emergency_halt(reason);
    } else {
        /* Generic halt */
        meow_log(MEOW_LOG_SCREECH, "HAL: No architecture-specific emergency halt - using generic halt");
        
        /* Disable interrupts if possible */
        if (g_current_hal_ops && g_current_hal_ops->cpu_ops && 
            g_current_hal_ops->cpu_ops->disable_interrupts) {
            g_current_hal_ops->cpu_ops->disable_interrupts();
        }

        /* Halt loop */
        while (1) {
            asm volatile("hlt");
        }
    }
}

/**
 * hal_panic - System panic with message
 */
void hal_panic(const char* message) {
    meow_log(MEOW_LOG_SCREECH, "==== HAL: KERNEL PANIC - %s ====", message ? message : "Unknown");

    /* Try to use architecture-specific panic */
    if (g_current_hal_ops && g_current_hal_ops->panic) {
        g_current_hal_ops->panic(message);
    } else {
        /* Use generic panic */
        meow_panic(message);
    }
}

/* ============================================================================
 * ARCHITECTURE REGISTRATION STUB FUNCTIONS
 * ============================================================================ */

/**
 * hal_register_arm64_ops - ARM64 operations registration stub
 * 
 * This is a stub function for ARM64 support. In a real implementation,
 * this would be in a separate ARM64-specific file.
 */
meow_error_t hal_register_arm64_ops(void) {
    meow_log(MEOW_LOG_HISS, "ARM64 HAL operations not implemented yet - cats are still learning ARM!");
    return MEOW_ERROR_NOT_SUPPORTED;
}

/* Note: hal_register_x86_ops() is implemented in hal_x86.c */