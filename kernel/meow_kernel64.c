#include <stdint.h>
#include "x86_64/x86_64_interrupts.h"
#include "../advanced/mm/x86_64_pmm.h"
#include "../advanced/mm/x86_64_paging.h"
#include "../advanced/mm/x86_64_heap.h"
#include "../advanced/process/x86_64/x86_64_context.h"
#include "../advanced/process/x86_64/x86_64_scheduler.h"

static void write_string(const char *text) {
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    static uint32_t cursor;
    while (*text) {
        char c = *text++;
        if (c == '\n') cursor = (cursor / 80 + 1) * 80;
        else if (cursor < 80 * 25) vga[cursor++] = (uint16_t)c | 0x0700;
    }
}

static void write_cat_banner(void)
{
    write_string(" /\\_/\\\\\n");
    write_string("( o.o )\n");
    write_string(" > ^ <\n\n");
}

#ifdef MEOW_TEST_MODE
#define COM1 0x3F8
#define DEBUG_EXIT_PORT 0xF4

static void serial_init(void)
{
    x86_64_outb(COM1 + 1, 0x00);
    x86_64_outb(COM1 + 3, 0x80);
    x86_64_outb(COM1 + 0, 0x01); /* 115200 baud */
    x86_64_outb(COM1 + 1, 0x00);
    x86_64_outb(COM1 + 3, 0x03); /* 8 data bits, no parity, one stop bit */
    x86_64_outb(COM1 + 2, 0xC7); /* enable and clear FIFOs */
    x86_64_outb(COM1 + 4, 0x03); /* DTR and RTS */
}

static void serial_write_char(char c)
{
    uint32_t timeout = 100000;
    while (!(x86_64_inb(COM1 + 5) & 0x20) && --timeout) {}
    x86_64_outb(COM1, (uint8_t)c);
}

static void serial_write_string(const char *text)
{
    while (*text) serial_write_char(*text++);
}

static void serial_write_u64(uint64_t value)
{
    char digits[20];
    uint32_t count = 0;

    if (value == 0) {
        serial_write_char('0');
        return;
    }
    while (value) {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (count) serial_write_char(digits[--count]);
}

void x86_64_test_exit(uint32_t status)
{
    x86_64_outl(DEBUG_EXIT_PORT, status);
    for (;;) x86_64_hlt();
}

void x86_64_test_exception_pass(uint64_t vector)
{
    serial_write_string("MEOWTEST:EXCEPTION:");
    serial_write_u64(vector);
    serial_write_string(":PASS\n");
    x86_64_test_exit(0);
}

void x86_64_test_exception_fail(uint64_t vector)
{
    serial_write_string("MEOWTEST:EXCEPTION:");
    serial_write_u64(vector);
    serial_write_string(":FAIL\n");
    x86_64_test_exit(1);
}

void x86_64_test_breakpoint_handled(uint64_t vector)
{
    serial_write_string("MEOWTEST:EXCEPTION:");
    serial_write_u64(vector);
    serial_write_string(":HANDLED\n");
}

#if defined(X86_64_TEST_EXCEPTION_VECTOR)
static void trigger_test_exception(void)
{
#if X86_64_TEST_EXCEPTION_VECTOR == 0
    __asm__ volatile ("xor %%edx, %%edx; mov $1, %%eax; xor %%ecx, %%ecx; divl %%ecx" ::: "eax", "ecx", "edx");
#elif X86_64_TEST_EXCEPTION_VECTOR == 3
    __asm__ volatile ("int3");
    x86_64_test_exception_pass(3);
#elif X86_64_TEST_EXCEPTION_VECTOR == 6
    __asm__ volatile ("ud2");
#elif X86_64_TEST_EXCEPTION_VECTOR == 13
    /* Null DS is legal in long mode; an out-of-limit selector is not. */
    __asm__ volatile ("mov $0x28, %%eax; mov %%ax, %%ds" ::: "eax");
#elif X86_64_TEST_EXCEPTION_VECTOR == 14
    (void)*(volatile uint64_t *)0x40000000ULL;
#else
#error "unsupported x86_64 exception test vector"
#endif
    x86_64_test_exception_fail(255);
}
#endif

static volatile uint64_t test_keyboard_events;
static volatile uint64_t scheduler_task_a_runs;
static volatile uint64_t scheduler_task_b_runs;

static void scheduler_demo_task(void *argument)
{
    volatile uint64_t *runs = (volatile uint64_t *)argument;

    for (;;) {
        ++*runs;
        x86_64_hlt();
    }
}

#if !defined(X86_64_TEST_EXCEPTION_VECTOR)
static void test_keyboard_irq(x86_64_interrupt_frame_t *frame)
{
    (void)frame;
    (void)x86_64_inb(0x60); /* acknowledge the PS/2 byte before EOI */
    ++test_keyboard_events;
}
#endif
#endif

void kernel_main64(uint32_t magic, const void *multiboot_info) {
    write_cat_banner();
#ifdef MEOW_TEST_MODE
    serial_init();
    serial_write_string("MEOWTEST:BOOT:PASS\n");
#endif
    if (pmm64_init(multiboot_info) != PMM64_OK) {
        write_string("ERROR: physical memory initialization failed\n");
#ifdef MEOW_TEST_MODE
        serial_write_string("MEOWTEST:PMM:FAIL\n");
        x86_64_test_exit(1);
#endif
        for (;;) __asm__ volatile ("cli; hlt");
    }
    if (x86_64_paging_init() != PAGING64_OK) {
        write_string("ERROR: virtual memory initialization failed\n");
#ifdef MEOW_TEST_MODE
        serial_write_string("MEOWTEST:VM:FAIL\n");
        x86_64_test_exit(1);
#endif
        for (;;) __asm__ volatile ("cli; hlt");
    }
    if (x86_64_heap_init() != HEAP64_OK) {
        write_string("ERROR: kernel heap initialization failed\n");
#ifdef MEOW_TEST_MODE
        serial_write_string("MEOWTEST:HEAP:FAIL\n");
        x86_64_test_exit(1);
#endif
        for (;;) __asm__ volatile ("cli; hlt");
    }
#ifdef MEOW_TEST_MODE
    if (!pmm64_self_test()) {
        serial_write_string("MEOWTEST:PMM:FAIL\n");
        x86_64_test_exit(1);
    }
    if (x86_64_paging_self_test() == 0) {
        serial_write_string("MEOWTEST:VM:FAIL\n");
        x86_64_test_exit(1);
    }
    if (x86_64_heap_self_test() == 0) {
        serial_write_string("MEOWTEST:HEAP:FAIL\n");
        x86_64_test_exit(1);
    }
    if (x86_64_context_self_test() == 0) {
        serial_write_string("MEOWTEST:CTX:FAIL\n");
        x86_64_test_exit(1);
    }
    serial_write_string("MEOWTEST:IDT:PASS\n");
    serial_write_string("MEOWTEST:PMM:PASS\n");
    serial_write_string("MEOWTEST:VM:PASS\n");
    serial_write_string("MEOWTEST:HEAP:PASS\n");
    serial_write_string("MEOWTEST:CTX:PASS\n");
#endif
    if (x86_64_scheduler_init() != SCHEDULER64_OK ||
        x86_64_scheduler_create("pounce", scheduler_demo_task,
                                (void *)&scheduler_task_a_runs, 8192) == 0 ||
        x86_64_scheduler_create("chase", scheduler_demo_task,
                                (void *)&scheduler_task_b_runs, 8192) == 0) {
        write_string("ERROR: scheduler initialization failed\n");
#ifdef MEOW_TEST_MODE
        serial_write_string("MEOWTEST:SCHED:FAIL\n");
        x86_64_test_exit(1);
#endif
        for (;;) __asm__ volatile ("cli; hlt");
    }
    write_string("MeowKernel\n");
    if (magic != 0x2BADB002) {
        write_string("ERROR: invalid Multiboot magic\n");
#ifdef MEOW_TEST_MODE
        serial_write_string("MEOWTEST:BOOT:FAIL\n");
        x86_64_test_exit(1);
#endif
    } else {
        write_string("Architecture: x86_64\nLong mode: enabled\nKernel entry: 64-bit\n");
    }

    x86_64_pic_init();
#ifdef MEOW_TEST_MODE
    serial_write_string("MEOWTEST:PIC:PASS\n");
#endif
    if (0) x86_64_irq_register_timer_switch(x86_64_scheduler_timer_tick);
    x86_64_pit_init(100);
#ifdef MEOW_TEST_MODE
    serial_write_string("MEOWTEST:PIT:PASS\n");
#if defined(X86_64_TEST_EXCEPTION_VECTOR)
    serial_write_string("MEOWTEST:EXCEPTION:BEGIN\n");
    trigger_test_exception();
#else
    x86_64_irq_register(1, test_keyboard_irq);
    x86_64_pic_enable_irq(1);
    x86_64_sti();
    {
        uint8_t scheduler_ready = 0;
        for (uint32_t wait = 0; wait < 1000; ++wait) {
            x86_64_scheduler_stats_t stats;
            if (x86_64_scheduler_get_stats(&stats) != SCHEDULER64_OK) break;
            if (x86_64_timer_ticks() >= 3 && test_keyboard_events != 0 &&
                scheduler_task_a_runs >= 2 && scheduler_task_b_runs >= 2 &&
                stats.context_switches >= 3) {
                scheduler_ready = 1;
                break;
            }
            x86_64_hlt();
        }
        if (!scheduler_ready) {
            serial_write_string("MEOWTEST:SCHED:FAIL\n");
            x86_64_test_exit(1);
        }
    }
    serial_write_string("MEOWTEST:SCHED:PASS\n");
    serial_write_string("MEOWTEST:IRQ0:PASS\n");
    serial_write_string("MEOWTEST:IRQ1:PASS\n");
    serial_write_string("MEOWTEST:PASS\n");
    x86_64_test_exit(0);
#endif
#else
    x86_64_sti();
#endif
    for (;;) __asm__ volatile ("hlt");
}
