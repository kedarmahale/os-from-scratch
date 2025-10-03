/* advanced/drivers/keyboard/meow_keyboard.c - PS/2 Keyboard Driver
 *                                             Implementation
 *
 * Copyright (c) 2025 MeowKernel Project  
 */

#include "meow_keyboard.h"
#include "../../kernel/meow_util.h"

/* ================================================================
 * INTERNAL STATE AND BUFFERS
 * ================================================================ */

#define KEYBOARD_BUFFER_SIZE 256

/* Keyboard state tracking */
static struct {
    uint8_t shift_pressed;
    uint8_t ctrl_pressed;
    uint8_t alt_pressed;
    uint8_t initialized;
} kb_state = {0};

/* Circular buffer for keyboard events */
static keyboard_event_t event_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_head = 0;
static volatile uint32_t buffer_tail = 0;
static volatile uint32_t buffer_count = 0;

/* Statistics */
static keyboard_stats_t kb_stats = {0};

/* Scancode to ASCII translation table (US layout) */
static const char scancode_ascii_map[] = {
    0,   0, '1', '2', '3', '4', '5', '6',     // 0x00-0x07
    '7', '8', '9', '0', '-', '=', '\b', '\t', // 0x08-0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  // 0x10-0x17
    'o', 'p', '[', ']', '\n', 0, 'a', 's',   // 0x18-0x1F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  // 0x20-0x27
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',  // 0x28-0x2F
    'b', 'n', 'm', ',', '.', '/', 0, '*',    // 0x30-0x37
    0, ' ', 0                                 // 0x38-0x3A
};

/* Shifted scancode to ASCII translation */
static const char scancode_ascii_shift_map[] = {
    0,   0, '!', '@', '#', '$', '%', '^',     // 0x00-0x07
    '&', '*', '(', ')', '_', '+', '\b', '\t', // 0x08-0x0F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',  // 0x10-0x17
    'O', 'P', '{', '}', '\n', 0, 'A', 'S',   // 0x18-0x1F
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',  // 0x20-0x27
    '"', '~', 0, '|', 'Z', 'X', 'C', 'V',    // 0x28-0x2F
    'B', 'N', 'M', '<', '>', '?', 0, '*',    // 0x30-0x37
    0, ' ', 0                                 // 0x38-0x3A
};

/* ================================================================
 * INTERNAL HELPER FUNCTIONS
 * ================================================================ */

static void add_event_to_buffer(const keyboard_event_t* event)
{
    if (buffer_count >= KEYBOARD_BUFFER_SIZE) {
        kb_stats.buffer_overflows++;
        meow_log(MEOW_LOG_HISS, " Keyboard buffer overflow - cats typing too fast!");
        return;
    }

    event_buffer[buffer_head] = *event;
    buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_count++;
}

static int get_event_from_buffer(keyboard_event_t* event)
{
    if (buffer_count == 0) {
        return 0;  /* No events available */
    }

    *event = event_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    buffer_count--;
    return 1;  /* Event retrieved */
}

/* ================================================================
 * IRQ HANDLER (Uses existing HAL interrupt system)
 * ================================================================ */

void keyboard_irq_handler(uint8_t irq)
{
    keyboard_event_t event = {0};
    
    /* Read status register using existing HAL I/O operations */
    uint8_t status = HAL_IO_OP(inb, PS2_STATUS_PORT);
    
    /* Check if data is available */
    if (!(status & PS2_STATUS_OUTPUT_FULL)) {
        return;  /* No data available */
    }
    
    /* Read scancode using existing HAL I/O operations */
    uint8_t scancode = HAL_IO_OP(inb, PS2_DATA_PORT);
    
    /* Check for key release */
    uint8_t key_released = scancode & KEY_RELEASE_MASK;
    uint8_t key_code = scancode & ~KEY_RELEASE_MASK;
    
    /* Update modifier key states */
    switch (key_code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            kb_state.shift_pressed = !key_released;
            break;
        case KEY_LCTRL:
            kb_state.ctrl_pressed = !key_released;
            break;
        case KEY_LALT:
            kb_state.alt_pressed = !key_released;
            break;
    }
    
    /* Create keyboard event */
    event.scancode = scancode;
    event.pressed = !key_released;
    event.shift = kb_state.shift_pressed;
    event.ctrl = kb_state.ctrl_pressed; 
    event.alt = kb_state.alt_pressed;
    event.ascii = scancode_to_ascii(key_code, kb_state.shift_pressed);
    
    /* Update statistics */
    if (key_released) {
        kb_stats.keys_released++;
    } else {
        kb_stats.keys_pressed++;
    }
    
    /* Add to buffer */
    add_event_to_buffer(&event);
    
    /* Log key press for debugging (only for actual key presses) */
    if (!key_released && event.ascii != 0) {
        meow_log(MEOW_LOG_PURR, " Key pressed: '%c' (scan: 0x%02X)", 
                 event.ascii, key_code);
    }
}

/* ================================================================
 * PUBLIC API IMPLEMENTATION
 * ================================================================ */

meow_error_t keyboard_init(void)
{
    meow_log(MEOW_LOG_MEOW, " Initializing PS/2 keyboard driver...");
    
    /* Clear state */
    kb_state.shift_pressed = 0;
    kb_state.ctrl_pressed = 0;
    kb_state.alt_pressed = 0;
    
    /* Clear buffer */
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
    
    /* Clear statistics */
    kb_stats.keys_pressed = 0;
    kb_stats.keys_released = 0;
    kb_stats.buffer_overflows = 0;
    kb_stats.invalid_scancodes = 0;
    
    /* Register IRQ handler using existing HAL interrupt system */
    meow_error_t result = HAL_INTERRUPT_OP(register_handler, 1, keyboard_irq_handler);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, " Failed to register keyboard IRQ handler: %s",
                 meow_error_to_string(result));
        return result;
    }
    
    /* Enable IRQ 1 using existing HAL */
    result = HAL_INTERRUPT_OP(enable_irq, 1);
    if (result != MEOW_SUCCESS) {
        meow_log(MEOW_LOG_YOWL, " Failed to enable keyboard IRQ: %s",
                 meow_error_to_string(result));
        return result;
    }
    
    /* Clear any pending keyboard data */
    while (HAL_IO_OP(inb, PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        HAL_IO_OP(inb, PS2_DATA_PORT);
    }
    
    kb_state.initialized = 1;
    
    meow_log(MEOW_LOG_CHIRP, " Keyboard driver initialized - ready to catch keystrokes!");
    return MEOW_SUCCESS;
}

meow_error_t keyboard_cleanup(void)
{
    if (!kb_state.initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    /* Disable IRQ 1 */
    HAL_INTERRUPT_OP(disable_irq, 1);
    
    /* Unregister handler */
    HAL_INTERRUPT_OP(unregister_handler, 1);
    
    kb_state.initialized = 0;
    
    meow_log(MEOW_LOG_PURR, " Keyboard driver cleaned up - no more typing for cats");
    return MEOW_SUCCESS;
}

meow_error_t keyboard_get_event(keyboard_event_t* event)
{
    MEOW_RETURN_IF_NULL(event);
    
    if (!kb_state.initialized) {
        return MEOW_ERROR_NOT_INITIALIZED;
    }
    
    if (get_event_from_buffer(event)) {
        return MEOW_SUCCESS;
    } 
    
    return MEOW_ERROR_IO_FAILURE;
}

char keyboard_wait_for_key(void)
{
    keyboard_event_t event;
    
    while (1) {
        if (keyboard_get_event(&event) == MEOW_SUCCESS) {
            if (event.pressed && event.ascii != 0) {
                return event.ascii;
            }
        }
        
        /* Use existing HAL halt operation */
        HAL_CPU_OP(halt);
    }
}

int keyboard_check_key(void)
{
    return buffer_count > 0;
}

meow_error_t keyboard_get_stats(keyboard_stats_t* stats)
{
    MEOW_RETURN_IF_NULL(stats);
    
    *stats = kb_stats;
    return MEOW_SUCCESS;
}

/* ================================================================
 * CONVENIENCE FUNCTIONS
 * ================================================================ */

char keyboard_getchar(void)
{
    return keyboard_wait_for_key();
}

int keyboard_gets(char* buffer, int max_len)
{
    MEOW_RETURN_VALUE_IF_NULL(buffer, 0);
    
    int pos = 0;
    char ch;
    
    while (pos < max_len - 1) {
        ch = keyboard_wait_for_key();
        
        if (ch == '\n' || ch == '\r') {
            break;  /* End of line */
        } else if (ch == '\b' && pos > 0) {
            pos--;  /* Backspace */
            meow_putc('\b');
            meow_putc(' ');
            meow_putc('\b');
        } else if (ch >= 32 && ch <= 126) {
            buffer[pos++] = ch;
            meow_putc(ch);  /* Echo character */
        }
    }
    
    buffer[pos] = '\0';
    meow_putc('\n');
    
    return pos;
}

void keyboard_clear_buffer(void)
{
    buffer_head = 0;
    buffer_tail = 0;
    buffer_count = 0;
    
    meow_log(MEOW_LOG_PURR, "🧹 Keyboard buffer cleared - fresh start for cats!");
}

/* ================================================================
 * SCANCODE TRANSLATION
 * ================================================================ */

char scancode_to_ascii(uint8_t scancode, uint8_t shift)
{
    if (scancode >= sizeof(scancode_ascii_map)) {
        kb_stats.invalid_scancodes++;
        return 0;
    }
    
    if (shift) {
        return scancode_ascii_shift_map[scancode];
    } else {
        return scancode_ascii_map[scancode];
    }
}