#include "x86_64_interrupts.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_FREQUENCY 1193180U

static x86_64_irq_handler_t irq_handlers[16];
static x86_64_irq_switch_handler_t timer_switch_handler;
static volatile uint64_t pit_ticks;

void x86_64_pic_init(void)
{
    /* Mask before and after reprogramming so no IRQ races initialization. */
    x86_64_outb(PIC1_DATA, 0xFF);
    x86_64_outb(PIC2_DATA, 0xFF);
    x86_64_outb(PIC1_COMMAND, 0x11);
    x86_64_io_wait();
    x86_64_outb(PIC2_COMMAND, 0x11);
    x86_64_io_wait();

    /* Keep hardware IRQs outside the CPU exception range. */
    x86_64_outb(PIC1_DATA, 32);
    x86_64_io_wait();
    x86_64_outb(PIC2_DATA, 40);
    x86_64_io_wait();

    x86_64_outb(PIC1_DATA, 4);
    x86_64_io_wait();
    x86_64_outb(PIC2_DATA, 2);
    x86_64_io_wait();

    x86_64_outb(PIC1_DATA, 0x01);
    x86_64_io_wait();
    x86_64_outb(PIC2_DATA, 0x01);
    x86_64_io_wait();

    /* Start masked; individual users must opt in to an IRQ. */
    x86_64_outb(PIC1_DATA, 0xFF);
    x86_64_outb(PIC2_DATA, 0xFF);
}

void x86_64_pic_enable_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;

    if (irq >= 16) return;
    port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    bit = irq < 8 ? irq : (uint8_t)(irq - 8);
    x86_64_outb(port, (uint8_t)(x86_64_inb(port) & ~(1U << bit)));
    if (irq >= 8) x86_64_pic_enable_irq(2);
}

void x86_64_pic_disable_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;

    if (irq >= 16) return;
    port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    bit = irq < 8 ? irq : (uint8_t)(irq - 8);
    x86_64_outb(port, (uint8_t)(x86_64_inb(port) | (1U << bit)));
}

void x86_64_pic_eoi(uint8_t irq)
{
    if (irq >= 8) x86_64_outb(PIC2_COMMAND, PIC_EOI);
    x86_64_outb(PIC1_COMMAND, PIC_EOI);
}

void x86_64_pit_init(uint32_t frequency)
{
    uint32_t divisor;

    if (frequency == 0) frequency = 100;
    divisor = PIT_BASE_FREQUENCY / frequency;
    if (divisor == 0) divisor = 1;
    if (divisor > 0xFFFF) divisor = 0xFFFF;

    x86_64_outb(PIT_COMMAND, 0x36); /* channel 0, lobyte/hibyte, mode 3 */
    x86_64_outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    x86_64_outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
    x86_64_pic_enable_irq(0);
}

void x86_64_irq_register(uint8_t irq, x86_64_irq_handler_t handler)
{
    if (irq < 16) irq_handlers[irq] = handler;
}

void x86_64_irq_register_timer_switch(x86_64_irq_switch_handler_t handler)
{
    timer_switch_handler = handler;
}

x86_64_interrupt_frame_t *x86_64_irq_dispatch(x86_64_interrupt_frame_t *frame)
{
    x86_64_interrupt_frame_t *return_frame = frame;
    uint64_t vector = frame ? frame->vector : 0;

    if (vector >= 32 && vector < 48) {
        uint8_t irq = (uint8_t)(vector - 32);
        if (irq == 0) ++pit_ticks;
        if (irq_handlers[irq]) irq_handlers[irq](frame);
        if (irq == 0 && timer_switch_handler) {
            x86_64_interrupt_frame_t *next_frame = timer_switch_handler(frame);
            if (next_frame) return_frame = next_frame;
        }
        x86_64_pic_eoi(irq);
    }
    return return_frame;
}

uint64_t x86_64_timer_ticks(void)
{
    return pit_ticks;
}
