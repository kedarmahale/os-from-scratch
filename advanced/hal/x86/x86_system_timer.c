/* advanced/hal/x86/x86_system_timer.c - Programmable Interval Timer
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "x86_meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* PIT I/O Ports */
#define PIT_CHANNEL0 0x40 /* Channel 0 data port (system timer) */
#define PIT_CHANNEL1 0x41 /* Channel 1 data port (RAM refresh) */
#define PIT_CHANNEL2 0x42 /* Channel 2 data port (PC speaker) */
#define PIT_COMMAND  0x43 /* Mode/Command register */

/* PIT Constants */
#define PIT_FREQUENCY 1193180 /* Base frequency in Hz */

/* Global timer state */
static uint32_t timer_frequency = 0;
static uint32_t timer_divisor = 0;

/* ============================================================================
 * PIT IMPLEMENTATION (Fixed function names and signatures)
 * ============================================================================ */

/* Initialize the PIT */
meow_error_t x86_pit_init(uint32_t frequency) {
    meow_log(MEOW_LOG_CHIRP, "x86: Initializing Programmable Interval Timer at %u Hz", frequency);

    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;

    /* Ensure divisor is within valid range */
    if (divisor > 65535) {
        divisor = 65535;
        frequency = PIT_FREQUENCY / divisor;
        meow_log(MEOW_LOG_HISS, "x86: PIT frequency adjusted to %u Hz (divisor limited)", frequency);
    } else if (divisor < 1) {
        divisor = 1;
        frequency = PIT_FREQUENCY;
        meow_log(MEOW_LOG_HISS, "x86: PIT frequency adjusted to %u Hz (minimum divisor)", frequency);
    }

    timer_frequency = frequency;
    timer_divisor = divisor;

    /* Send command byte to PIT */
    /* Channel 0, Access mode: lobyte/hibyte, Mode 3: square wave generator, Binary mode */
    x86_outb(PIT_COMMAND, 0x36);

    /* Send frequency divisor (lobyte first, then hibyte) */
    x86_outb(PIT_CHANNEL0, divisor & 0xFF);        /* Low byte */
    x86_outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); /* High byte */

    /* Enable timer IRQ (IRQ 0) */
    x86_pic_enable_irq(0);

    meow_log(MEOW_LOG_CHIRP, "x86: PIT configured (divisor: %u, actual frequency: %u Hz)",
             divisor, frequency);
    
    return MEOW_SUCCESS;
}

/* Get current timer frequency */
uint32_t x86_pit_get_frequency(void) {
    return timer_frequency;
}

/* Get current timer divisor */
uint32_t x86_pit_get_divisor(void) {
    return timer_divisor;
}

/* Sleep for specified milliseconds using timer ticks */
meow_error_t x86_pit_sleep(uint32_t milliseconds) {
    if (timer_frequency == 0) {
        meow_log(MEOW_LOG_YOWL, "x86: PIT not initialized, cannot sleep");
        return MEOW_ERROR_NOT_INITIALIZED;
    }

    /* For now, just use a simple busy wait with halt */
    uint32_t target_loops = milliseconds * 1000; /* Rough approximation */
    
    meow_log(MEOW_LOG_PURR, "x86: Sleeping for %u ms", milliseconds);
    
    for (uint32_t i = 0; i < target_loops; i++) {
        x86_hlt(); /* Halt until next interrupt */
    }
    
    return MEOW_SUCCESS;
}

/* Enable/disable PIT channel */
meow_error_t x86_pit_set_enabled(uint8_t enabled) {
    if (enabled) {
        meow_error_t result = x86_pic_enable_irq(0);
        if (result == MEOW_SUCCESS) {
            meow_log(MEOW_LOG_MEOW, "x86: PIT timer enabled");
        }
        return result;
    } else {
        meow_error_t result = x86_pic_disable_irq(0);
        if (result == MEOW_SUCCESS) {
            meow_log(MEOW_LOG_MEOW, "x86: PIT timer disabled");
        }
        return result;
    }
}