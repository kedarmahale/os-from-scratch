/* advanced/hal/x86/pit.c - Programmable Interval Timer */

#include "hal_x86.h"

/* Forward declaration for meow_printf */
extern void meow_printf(const char* format, ...);

/* PIT I/O Ports */
#define PIT_CHANNEL0    0x40    /* Channel 0 data port (system timer) */
#define PIT_CHANNEL1    0x41    /* Channel 1 data port (RAM refresh) */
#define PIT_CHANNEL2    0x42    /* Channel 2 data port (PC speaker) */
#define PIT_COMMAND     0x43    /* Mode/Command register */

/* PIT Constants */
#define PIT_FREQUENCY   1193180 /* Base frequency in Hz */

/* Global timer state */
static uint32_t timer_frequency = 0;
static uint32_t timer_divisor = 0;

/* Initialize the PIT */
void x86_pit_init(uint32_t frequency) {
    meow_printf("x86: Initializing Programmable Interval Timer at %u Hz\n", frequency);
    
    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Ensure divisor is within valid range */
    if (divisor > 65535) {
        divisor = 65535;
        frequency = PIT_FREQUENCY / divisor;
        meow_printf("x86: PIT frequency adjusted to %u Hz (divisor limited)\n", frequency);
    } else if (divisor < 1) {
        divisor = 1;
        frequency = PIT_FREQUENCY;
        meow_printf("x86: PIT frequency adjusted to %u Hz (minimum divisor)\n", frequency);
    }

    timer_frequency = frequency;
    timer_divisor = divisor;

    /* Send command byte to PIT */
    /* Channel 0, Access mode: lobyte/hibyte, Mode 3: square wave generator, Binary mode */
    hal_port_outb(PIT_COMMAND, 0x36);

    /* Send frequency divisor (lobyte first, then hibyte) */
    hal_port_outb(PIT_CHANNEL0, divisor & 0xFF);        /* Low byte */
    hal_port_outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); /* High byte */

    /* Enable timer IRQ (IRQ 0) */
    x86_pic_enable_irq(0);

    meow_printf("x86: PIT configured (divisor: %u, actual frequency: %u Hz)\n", 
            divisor, frequency);
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
void x86_pit_sleep(uint32_t milliseconds) {
    if (timer_frequency == 0) {
        meow_printf("x86: PIT not initialized, cannot sleep\n");
        return;
    }
    
    uint64_t start_ticks = hal_timer_get_ticks();
    uint64_t target_ticks = start_ticks + ((uint64_t)milliseconds * timer_frequency) / 1000;
    
    meow_printf("x86: Sleeping for %u ms (%llu ticks)\n", milliseconds, target_ticks - start_ticks);
    
    while (hal_timer_get_ticks() < target_ticks) {
        x86_hlt(); /* Halt until next interrupt */
    }
}

/* Enable/disable PIT channel */
void x86_pit_set_enabled(bool enabled) {
    if (enabled) {
        x86_pic_enable_irq(0);
        meow_printf("x86: PIT timer enabled\n");
    } else {
        x86_pic_disable_irq(0);
        meow_printf("x86: PIT timer disabled\n");
    }
}
