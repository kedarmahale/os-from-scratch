/* advanced/hal/x86/pic.c - Programmable Interrupt Controller */

#include "hal_x86.h"

/* Forward declaration for kprintf */
extern void meow_printf(const char* format, ...);

/* PIC I/O Ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC Commands */
#define PIC_EOI         0x20    /* End of interrupt command */

/* Initialization Command Words (ICW) */
#define ICW1_ICW4       0x01    /* ICW4 needed */
#define ICW1_SINGLE     0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08    /* Level triggered (edge) mode */
#define ICW1_INIT       0x10    /* Initialization required */

#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested (not) */

/* Initialize the PIC */
void x86_pic_init(void) {
    meow_printf("x86: Initializing Programmable Interrupt Controller\n");
    
    /* Save current interrupt masks */
    uint8_t mask1 = hal_port_inb(PIC1_DATA);
    uint8_t mask2 = hal_port_inb(PIC2_DATA);

    /* Start initialization sequence in cascade mode */
    hal_port_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    hal_port_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    /* ICW2: Set vector offsets */
    hal_port_outb(PIC1_DATA, 32);   /* Master PIC vector offset (IRQ 0-7 -> INT 32-39) */
    hal_port_outb(PIC2_DATA, 40);   /* Slave PIC vector offset (IRQ 8-15 -> INT 40-47) */

    /* ICW3: Set up cascade - tell master PIC that slave is at IRQ2 (0000 0100) */
    hal_port_outb(PIC1_DATA, 4);
    /* Tell slave PIC its cascade identity (0000 0010) */
    hal_port_outb(PIC2_DATA, 2);

    /* ICW4: Set mode */
    hal_port_outb(PIC1_DATA, ICW4_8086);
    hal_port_outb(PIC2_DATA, ICW4_8086);

    /* Restore saved masks (or start with all IRQs disabled) */
    hal_port_outb(PIC1_DATA, 0xFF);  /* Mask all IRQs initially */
    hal_port_outb(PIC2_DATA, 0xFF);

    meow_printf("x86: PIC initialized (Master: INT 32-39, Slave: INT 40-47)\n");
}

/* Send End-of-Interrupt signal */
void x86_pic_eoi(uint8_t irq) {
    /* If the IRQ came from the slave PIC, send EOI to both PICs */
    if (irq >= 8) {
        hal_port_outb(PIC2_COMMAND, PIC_EOI);
    }
    
    /* Always send EOI to master PIC */
    hal_port_outb(PIC1_COMMAND, PIC_EOI);
}

/* Enable a specific IRQ */
void x86_pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = hal_port_inb(port) & ~(1 << irq);
    hal_port_outb(port, value);
}

/* Disable a specific IRQ */
void x86_pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = hal_port_inb(port) | (1 << irq);
    hal_port_outb(port, value);
}

/* Get IRQ mask for debugging */
uint16_t x86_pic_get_mask(void) {
    return (hal_port_inb(PIC2_DATA) << 8) | hal_port_inb(PIC1_DATA);
}

/* Set IRQ mask */
void x86_pic_set_mask(uint16_t mask) {
    hal_port_outb(PIC1_DATA, mask & 0xFF);
    hal_port_outb(PIC2_DATA, (mask >> 8) & 0xFF);
}
