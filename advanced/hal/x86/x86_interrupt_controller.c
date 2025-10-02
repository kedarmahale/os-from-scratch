/* advanced/hal/x86/x86_interrupt_controller.c - Programmable Interrupt 
 *                                               Controller
 *
 * Copyright (c) 2025 MeowKernel Project
 */

#include "x86_meow_hal_interface.h"
#include "../../kernel/meow_util.h"

/* PIC I/O Ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* PIC Commands */
#define PIC_EOI      0x20 /* End of interrupt command */

/* Initialization Command Words (ICW) */
#define ICW1_ICW4      0x01 /* ICW4 needed */
#define ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define ICW1_INIT      0x10 /* Initialization required */

#define ICW4_8086      0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO      0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM      0x10 /* Special fully nested (not) */

/* ============================================================================
 * PIC IMPLEMENTATION (Using corrected function names and return types)
 * ============================================================================ */

/* Initialize the PIC */
meow_error_t x86_pic_init(void) {
    meow_log(MEOW_LOG_CHIRP, "x86: Initializing Programmable Interrupt Controller");

    /* Save current interrupt masks for restoration if needed */
    uint8_t mask1 = x86_inb(PIC1_DATA);
    uint8_t mask2 = x86_inb(PIC2_DATA);
    
    /* Avoid unused variable warnings by using the masks */
    (void)mask1; /* We could restore these later if needed */
    (void)mask2;

    /* Start initialization sequence in cascade mode */
    x86_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    x86_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    /* ICW2: Set vector offsets */
    x86_outb(PIC1_DATA, 32); /* Master PIC vector offset (IRQ 0-7 -> INT 32-39) */
    x86_outb(PIC2_DATA, 40); /* Slave PIC vector offset (IRQ 8-15 -> INT 40-47) */

    /* ICW3: Set up cascade - tell master PIC that slave is at IRQ2 (0000 0100) */
    x86_outb(PIC1_DATA, 4);
    /* Tell slave PIC its cascade identity (0000 0010) */
    x86_outb(PIC2_DATA, 2);

    /* ICW4: Set mode */
    x86_outb(PIC1_DATA, ICW4_8086);
    x86_outb(PIC2_DATA, ICW4_8086);

    /* Start with all IRQs disabled initially for safety */
    x86_outb(PIC1_DATA, 0xFF); /* Mask all IRQs initially */
    x86_outb(PIC2_DATA, 0xFF);

    meow_log(MEOW_LOG_CHIRP, "x86: PIC initialized (Master: INT 32-39, Slave: INT 40-47)");
    return MEOW_SUCCESS;
}

/* Remap PIC to avoid conflicts with CPU exceptions */
meow_error_t x86_pic_remap(uint8_t offset1, uint8_t offset2) {
    meow_log(MEOW_LOG_MEOW, "x86: Remapping PIC (Master: %d, Slave: %d)", offset1, offset2);

    /* Save current masks */
    uint8_t mask1 = x86_inb(PIC1_DATA);
    uint8_t mask2 = x86_inb(PIC2_DATA);

    /* Start initialization sequence */
    x86_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    x86_io_wait();
    x86_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    x86_io_wait();

    /* Set vector offsets */
    x86_outb(PIC1_DATA, offset1);
    x86_io_wait();
    x86_outb(PIC2_DATA, offset2);
    x86_io_wait();

    /* Set up cascade */
    x86_outb(PIC1_DATA, 4);  /* Tell master about slave at IRQ2 */
    x86_io_wait();
    x86_outb(PIC2_DATA, 2);  /* Tell slave its cascade identity */
    x86_io_wait();

    /* Set mode */
    x86_outb(PIC1_DATA, ICW4_8086);
    x86_io_wait();
    x86_outb(PIC2_DATA, ICW4_8086);
    x86_io_wait();

    /* Restore masks */
    x86_outb(PIC1_DATA, mask1);
    x86_outb(PIC2_DATA, mask2);

    return MEOW_SUCCESS;
}

/* Send End-of-Interrupt signal */
void x86_pic_eoi(uint8_t irq) {
    /* If the IRQ came from the slave PIC, send EOI to both PICs */
    if (irq >= 8) {
        x86_outb(PIC2_COMMAND, PIC_EOI);
    }

    /* Always send EOI to master PIC */
    x86_outb(PIC1_COMMAND, PIC_EOI);
}

/* Enable a specific IRQ - FIXED return type */
meow_error_t x86_pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq >= 16) {
        meow_log(MEOW_LOG_YOWL, "x86: Invalid IRQ number: %u", irq);
        return MEOW_ERROR_INVALID_PARAMETER;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = x86_inb(port) & ~(1 << irq);
    x86_outb(port, value);

    meow_log(MEOW_LOG_MEOW, "x86: Enabled IRQ %u", (irq < 8) ? irq : irq + 8);
    return MEOW_SUCCESS;
}

/* Disable a specific IRQ - FIXED return type */
meow_error_t x86_pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq >= 16) {
        meow_log(MEOW_LOG_YOWL, "x86: Invalid IRQ number: %u", irq);
        return MEOW_ERROR_INVALID_PARAMETER;
    }

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = x86_inb(port) | (1 << irq);
    x86_outb(port, value);

    meow_log(MEOW_LOG_MEOW, "x86: Disabled IRQ %u", (irq < 8) ? irq : irq + 8);
    return MEOW_SUCCESS;
}

/* Disable all IRQs */
meow_error_t x86_pic_disable_all_irqs(void) {
    meow_log(MEOW_LOG_MEOW, "x86: Disabling all IRQs");
    
    x86_outb(PIC1_DATA, 0xFF);
    x86_outb(PIC2_DATA, 0xFF);
    
    return MEOW_SUCCESS;
}

/* Get IRQ mask for debugging */
uint16_t x86_pic_get_mask(void) {
    return (x86_inb(PIC2_DATA) << 8) | x86_inb(PIC1_DATA);
}

/* Set IRQ mask - FIXED return type */
meow_error_t x86_pic_set_mask(uint16_t mask) {
    meow_log(MEOW_LOG_MEOW, "x86: Setting PIC mask to 0x%04x", mask);
    
    x86_outb(PIC1_DATA, mask & 0xFF);
    x86_outb(PIC2_DATA, (mask >> 8) & 0xFF);
    
    return MEOW_SUCCESS;
}