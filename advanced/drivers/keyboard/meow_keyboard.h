/* advanced/drivers/keyboard/meow_keyboard.h - PS/2 Keyboard Driver Interface
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_KEYBOARD_H
#define MEOW_KEYBOARD_H

#include <stdint.h>
#include "../../kernel/meow_error_definitions.h"
#include "../../hal/meow_hal_interface.h"

/* ================================================================
 * KEYBOARD HARDWARE CONSTANTS
 * ================================================================ */

/* PS/2 Controller ports */
#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

/* Status register flags */
#define PS2_STATUS_OUTPUT_FULL   0x01
#define PS2_STATUS_INPUT_FULL    0x02

/* Common scan codes */
#define KEY_ESC         0x01
#define KEY_ENTER       0x1C
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_LCTRL       0x1D
#define KEY_LALT        0x38
#define KEY_SPACE       0x39
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F

/* Key release mask */
#define KEY_RELEASE_MASK    0x80

/* ================================================================
 * KEYBOARD DRIVER INTERFACE
 * ================================================================ */

/* Keyboard event structure */
typedef struct keyboard_event {
    uint8_t scancode;
    uint8_t ascii;
    uint8_t pressed;     /* 1 if pressed, 0 if released */
    uint8_t shift;
    uint8_t ctrl;
    uint8_t alt;
} keyboard_event_t;

/* Keyboard statistics */
typedef struct keyboard_stats {
    uint32_t keys_pressed;
    uint32_t keys_released;
    uint32_t buffer_overflows;
    uint32_t invalid_scancodes;
} keyboard_stats_t;

/* ================================================================
 * DRIVER FUNCTIONS (Build on existing HAL)
 * ================================================================ */

/**
 * keyboard_init - Initialize PS/2 keyboard driver
 * 
 * Uses existing HAL_INTERRUPT_OP to register IRQ 1 handler
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t keyboard_init(void);

/**
 * keyboard_cleanup - Cleanup keyboard driver
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t keyboard_cleanup(void);

/**
 * keyboard_get_event - Get next keyboard event (non-blocking)
 * @event: Output keyboard event structure
 * 
 * @return MEOW_SUCCESS if event available, MEOW_ERROR_NO_DATA if none
 */
meow_error_t keyboard_get_event(keyboard_event_t* event);

/**
 * keyboard_wait_for_key - Wait for next key press (blocking)
 * 
 * @return ASCII code of key pressed, or 0 for non-ASCII keys
 */
char keyboard_wait_for_key(void);

/**
 * keyboard_check_key - Check if key is available (non-blocking)
 * 
 * @return 1 if key available, 0 if not
 */
int keyboard_check_key(void);

/**
 * keyboard_get_stats - Get keyboard driver statistics
 * @stats: Output statistics structure
 * 
 * @return MEOW_SUCCESS or error code
 */
meow_error_t keyboard_get_stats(keyboard_stats_t* stats);

/* ================================================================
 * CONVENIENCE FUNCTIONS
 * ================================================================ */

/**
 * keyboard_gets - Read a line from keyboard
 * @buffer: Output buffer
 * @max_len: Maximum buffer length
 * 
 * @return Number of characters read
 */
int keyboard_gets(char* buffer, int max_len);

/**
 * keyboard_getchar - Get single character (blocking)
 * 
 * @return Character code
 */
char keyboard_getchar(void);

/**
 * keyboard_clear_buffer - Clear internal keyboard buffer
 */
void keyboard_clear_buffer(void);

/* ================================================================
 * INTERNAL FUNCTIONS (Do not call directly)
 * ================================================================ */

/* IRQ handler - registered with existing HAL interrupt system */
void keyboard_irq_handler(uint8_t irq);

/* Scancode to ASCII conversion */
char scancode_to_ascii(uint8_t scancode, uint8_t shift);

#endif /* MEOW_KEYBOARD_H */