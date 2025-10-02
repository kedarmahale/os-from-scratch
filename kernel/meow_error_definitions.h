/* kernel/meow_error_definitions.h - MeowKernel Centralized Error Codes
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_ERROR_DEFINITIONS_H
#define MEOW_ERROR_DEFINITIONS_H

#include <stdint.h>

/* ============================================================================
 * MEOWKERNEL ERROR CODES (AS MACROS)
 * ============================================================================ */

/* Success and general error codes */
#define MEOW_SUCCESS                        0
#define MEOW_ERROR_GENERAL                 -1
#define MEOW_ERROR_UNKNOWN                 -2

/* Parameter and validation errors (Category: -10 to -19) */
#define MEOW_ERROR_NULL_POINTER           -10
#define MEOW_ERROR_INVALID_PARAMETER      -11
#define MEOW_ERROR_INVALID_SIZE           -12
#define MEOW_ERROR_INVALID_ALIGNMENT      -13
#define MEOW_ERROR_BUFFER_TOO_SMALL       -14
#define MEOW_ERROR_INVALID_STATE          -15
#define MEOW_ERROR_INVALID_HANDLE         -16

/* Memory management errors (Category: -20 to -29) */
#define MEOW_ERROR_OUT_OF_MEMORY          -20
#define MEOW_ERROR_MEMORY_CORRUPTION      -21
#define MEOW_ERROR_DOUBLE_FREE            -22
#define MEOW_ERROR_HEAP_EXHAUSTED         -23
#define MEOW_ERROR_BAD_ALLOCATION         -24
#define MEOW_ERROR_MEMORY_LEAK            -25

/* Hardware and initialization errors (Category: -30 to -39) */
#define MEOW_ERROR_HARDWARE_FAILURE       -30
#define MEOW_ERROR_NOT_INITIALIZED        -31
#define MEOW_ERROR_ALREADY_INITIALIZED    -32
#define MEOW_ERROR_INITIALIZATION_FAILED  -33
#define MEOW_ERROR_DEVICE_NOT_FOUND       -34
#define MEOW_ERROR_DEVICE_BUSY            -35

/* System and resource errors (Category: -40 to -49) */
#define MEOW_ERROR_TIMEOUT                -40
#define MEOW_ERROR_NOT_SUPPORTED          -41
#define MEOW_ERROR_ACCESS_DENIED          -42
#define MEOW_ERROR_RESOURCE_EXHAUSTED     -43
#define MEOW_ERROR_SYSTEM_LIMIT           -44
#define MEOW_ERROR_QUOTA_EXCEEDED         -45

/* I/O and communication errors (Category: -50 to -59) */
#define MEOW_ERROR_IO_FAILURE             -50
#define MEOW_ERROR_READ_FAILURE           -51
#define MEOW_ERROR_WRITE_FAILURE          -52
#define MEOW_ERROR_SEEK_FAILURE           -53
#define MEOW_ERROR_CONNECTION_LOST        -54
#define MEOW_ERROR_PROTOCOL_ERROR         -55

/* ============================================================================
 * ERROR CODE TYPE AND UTILITIES
 * ============================================================================ */

/* Error code type - consistent 32-bit signed integer */
typedef int32_t meow_error_t;

/* ============================================================================
 * ERROR CHECKING MACROS
 * ============================================================================ */

/* Check if an error code indicates success */
#define MEOW_IS_SUCCESS(err)        ((err) == MEOW_SUCCESS)
#define MEOW_IS_ERROR(err)          ((err) != MEOW_SUCCESS)

/* Check for specific error categories */
#define MEOW_IS_PARAM_ERROR(err)    ((err) <= -10 && (err) >= -19)
#define MEOW_IS_MEMORY_ERROR(err)   ((err) <= -20 && (err) >= -29)
#define MEOW_IS_HARDWARE_ERROR(err) ((err) <= -30 && (err) >= -39)
#define MEOW_IS_SYSTEM_ERROR(err)   ((err) <= -40 && (err) >= -49)
#define MEOW_IS_IO_ERROR(err)       ((err) <= -50 && (err) >= -59)

/* ============================================================================
 * ERROR HANDLING MACROS
 * ============================================================================ */

/* Return if error - for functions returning meow_error_t */
#define MEOW_RETURN_IF_ERROR(expr) do { \
    meow_error_t _err = (expr); \
    if (MEOW_IS_ERROR(_err)) { \
        return _err; \
    } \
} while(0)

/* Return specific value if error */
#define MEOW_RETURN_VALUE_IF_ERROR(expr, retval) do { \
    meow_error_t _err = (expr); \
    if (MEOW_IS_ERROR(_err)) { \
        return (retval); \
    } \
} while(0)

/* Return if null pointer */
#define MEOW_RETURN_IF_NULL(ptr) do { \
    if (!(ptr)) { \
        return MEOW_ERROR_NULL_POINTER; \
    } \
} while(0)

/* Return specific value if null pointer */
#define MEOW_RETURN_VALUE_IF_NULL(ptr, retval) do { \
    if (!(ptr)) { \
        return (retval); \
    } \
} while(0)

/* ============================================================================
 * ERROR MESSAGE FUNCTIONS
 * ============================================================================ */

/**
 * meow_error_to_string - Convert error code to human-readable string
 * @error: Error code to convert
 * 
 * Returns a constant string describing the error. The string is always
 * valid and does not need to be freed.
 * 
 * @return Constant string describing the error
 */
const char* meow_error_to_string(meow_error_t error);

/**
 * meow_error_get_category - Get error category name
 * @error: Error code
 * 
 * Returns the category name for the error (e.g., "Memory", "Hardware").
 * 
 * @return Constant string with category name
 */
const char* meow_error_get_category(meow_error_t error);

/**
 * meow_error_is_recoverable - Check if error is potentially recoverable
 * @error: Error code to check
 * 
 * Determines if an error condition might be recoverable through retry
 * or alternative approaches.
 * 
 * @return 1 if potentially recoverable, 0 if fatal
 */
uint8_t meow_error_is_recoverable(meow_error_t error);

/* ============================================================================
 * ASSERTION MACROS WITH ERROR CODES
 * ============================================================================ */

#ifdef DEBUG
#define MEOW_ASSERT(condition, error_code) do { \
    if (!(condition)) { \
        meow_log(MEOW_LOG_SCREECH, "ASSERTION FAILED: %s at %s:%d - %s", \
                 #condition, __FILE__, __LINE__, \
                 meow_error_to_string(error_code)); \
        meow_panic("Cat assertion failure"); \
    } \
} while(0)
#else
#define MEOW_ASSERT(condition, error_code) ((void)0)
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/* Forward declaration for logging integration */
typedef enum meow_log_level meow_log_level_t;
void meow_log(meow_log_level_t level, const char* format, ...);
void meow_panic(const char* message);

#endif /* MEOW_ERROR_DEFINITIONS_H */