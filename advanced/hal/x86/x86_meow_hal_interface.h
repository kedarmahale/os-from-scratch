/* advanced/hal/x86/x86_meow_hal_interface.h - x86 Architecture HAL Interface
 *
 * x86-specific hardware abstraction layer interface and low-level functions
 * Copyright (c) 2025 MeowKernel Project
 */

#ifndef MEOW_HAL_X86_H
#define MEOW_HAL_X86_H

#include "../meow_hal_interface.h"

/* ============================================================================
 * X86-SPECIFIC CONSTANTS AND STRUCTURES
 * ============================================================================ */

/* x86 Memory Layout Constants */
#define X86_KERNEL_VIRTUAL_BASE     0xC0000000  /* 3GB */
#define X86_KERNEL_PHYSICAL_BASE    0x00100000  /* 1MB */
#define X86_PAGE_SIZE               4096
#define X86_PAGE_ALIGN_MASK         0xFFFFF000

/* x86 GDT Constants */
#define X86_GDT_ENTRIES             8
#define X86_GDT_NULL_SELECTOR       0x00
#define X86_GDT_KERNEL_CODE         0x08
#define X86_GDT_KERNEL_DATA         0x10
#define X86_GDT_USER_CODE           0x18
#define X86_GDT_USER_DATA           0x20
#define X86_GDT_TSS_SELECTOR        0x28

/* x86 IDT Constants */
#define X86_IDT_ENTRIES             256
#define X86_IDT_GATE_TASK           0x85
#define X86_IDT_GATE_INTERRUPT      0x8E
#define X86_IDT_GATE_TRAP           0x8F

/* x86 PIC Constants */
#define X86_PIC1_COMMAND            0x20
#define X86_PIC1_DATA               0x21
#define X86_PIC2_COMMAND            0xA0
#define X86_PIC2_DATA               0xA1
#define X86_PIC_EOI                 0x20

/* x86 PIT Constants */
#define X86_PIT_CHANNEL0            0x40
#define X86_PIT_CHANNEL1            0x41
#define X86_PIT_CHANNEL2            0x42
#define X86_PIT_COMMAND             0x43
#define X86_PIT_FREQUENCY           1193180

/* VGA Text Mode Constants */
#define X86_VGA_MEMORY              0xB8000
#define X86_VGA_WIDTH               80
#define X86_VGA_HEIGHT              25
#define X86_VGA_COLOR_BLACK         0
#define X86_VGA_COLOR_BLUE          1
#define X86_VGA_COLOR_GREEN         2
#define X86_VGA_COLOR_CYAN          3
#define X86_VGA_COLOR_RED           4
#define X86_VGA_COLOR_MAGENTA       5
#define X86_VGA_COLOR_BROWN         6
#define X86_VGA_COLOR_LIGHT_GRAY    7
#define X86_VGA_COLOR_DARK_GRAY     8
#define X86_VGA_COLOR_LIGHT_BLUE    9
#define X86_VGA_COLOR_LIGHT_GREEN   10
#define X86_VGA_COLOR_LIGHT_CYAN    11
#define X86_VGA_COLOR_LIGHT_RED     12
#define X86_VGA_COLOR_LIGHT_MAGENTA 13
#define X86_VGA_COLOR_LIGHT_BROWN   14
#define X86_VGA_COLOR_WHITE         15

/* ============================================================================
 * X86-SPECIFIC DATA STRUCTURES
 * ============================================================================ */

/* GDT Entry Structure */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) x86_gdt_entry_t;

/* GDT Pointer Structure */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) x86_gdt_ptr_t;

/* IDT Entry Structure */
typedef struct {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) x86_idt_entry_t;

/* IDT Pointer Structure */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) x86_idt_ptr_t;

/* CPU State Structure (for interrupt handling) */
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pusha */
    uint32_t ds, es, fs, gs;                          /* segment selectors */
    uint32_t interrupt_number, error_code;            /* interrupt info */
    uint32_t eip, cs, eflags, user_esp, ss;          /* pushed by processor */
} __attribute__((packed)) x86_cpu_state_t;

/* ============================================================================
 * X86 LOW-LEVEL HARDWARE FUNCTIONS
 * ============================================================================ */

/* Assembly helper functions (implemented in assembly) */
extern void x86_gdt_flush(uint32_t gdt_ptr);
extern void x86_idt_flush(uint32_t idt_ptr);
extern void x86_tss_flush(void);

/* CPU control inline functions */
static inline void x86_cli(void) {
    asm volatile("cli" ::: "memory");
}

static inline void x86_sti(void) {
    asm volatile("sti" ::: "memory");
}

static inline void x86_hlt(void) {
    asm volatile("hlt" ::: "memory");
}

static inline void x86_nop(void) {
    asm volatile("nop");
}

/* Control register access */
static inline uint32_t x86_get_cr0(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

static inline void x86_set_cr0(uint32_t cr0) {
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline uint32_t x86_get_cr2(void) {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

static inline uint32_t x86_get_cr3(void) {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void x86_set_cr3(uint32_t cr3) {
    asm volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static inline uint32_t x86_get_eflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

static inline void x86_set_eflags(uint32_t eflags) {
    asm volatile("pushl %0; popfl" :: "r"(eflags) : "memory", "cc");
}

/* Port I/O functions */
static inline uint8_t x86_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline uint16_t x86_inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline uint32_t x86_inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void x86_outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" :: "a"(data), "Nd"(port));
}

static inline void x86_outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1" :: "a"(data), "Nd"(port));
}

static inline void x86_outl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %1" :: "a"(data), "Nd"(port));
}

/* I/O wait function */
static inline void x86_io_wait(void) {
    x86_outb(0x80, 0);
}

/* Memory fence functions */
static inline void x86_memory_barrier(void) {
    asm volatile("mfence" ::: "memory");
}

static inline void x86_memory_barrier_read(void) {
    asm volatile("lfence" ::: "memory");
}

static inline void x86_memory_barrier_write(void) {
    asm volatile("sfence" ::: "memory");
}

/* ============================================================================
 * X86 SUBSYSTEM INITIALIZATION FUNCTIONS
 * ============================================================================ */

/* GDT (Global Descriptor Table) management */
meow_error_t x86_gdt_init(void);
meow_error_t x86_gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, 
                               uint8_t access, uint8_t granularity);
uint32_t x86_gdt_get_selector(uint32_t index);

/* IDT (Interrupt Descriptor Table) management */
meow_error_t x86_idt_init(void);
meow_error_t x86_idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, 
                               uint8_t flags);
void x86_idt_handle_interrupt(x86_cpu_state_t* state);

/* PIC (Programmable Interrupt Controller) management */
meow_error_t x86_pic_init(void);
meow_error_t x86_pic_remap(uint8_t offset1, uint8_t offset2);
void x86_pic_eoi(uint8_t irq);
meow_error_t x86_pic_enable_irq(uint8_t irq);
meow_error_t x86_pic_disable_irq(uint8_t irq);
meow_error_t x86_pic_disable_all_irqs(void);
uint16_t x86_pic_get_mask(void);
meow_error_t x86_pic_set_mask(uint16_t mask);

/* PIT (Programmable Interval Timer) management */
meow_error_t x86_pit_init(uint32_t frequency);
meow_error_t x86_pit_set_frequency(uint32_t frequency);
uint32_t x86_pit_get_frequency(void);
meow_error_t x86_pit_set_mode(uint8_t channel, uint8_t mode);

/* ============================================================================
 * X86 MEMORY MANAGEMENT FUNCTIONS
 * ============================================================================ */

/* Memory detection and management */
uint32_t x86_detect_memory_from_multiboot(const multiboot_info_t* mbi);
uint32_t x86_get_kernel_memory_usage(void);
meow_error_t x86_memory_map_init(multiboot_info_t* mbi);

/* Physical memory management */
void* x86_physical_alloc_page(void);
meow_error_t x86_physical_free_page(void* page);
uint32_t x86_physical_get_total_pages(void);
uint32_t x86_physical_get_free_pages(void);

/* Virtual memory management (paging) */
meow_error_t x86_paging_init(void);
meow_error_t x86_paging_enable(void);
meow_error_t x86_paging_disable(void);
meow_error_t x86_paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, 
                                  uint32_t flags);
meow_error_t x86_paging_unmap_page(uint32_t virtual_addr);
uint32_t x86_paging_get_physical_addr(uint32_t virtual_addr);

/* ============================================================================
 * X86 CPU FEATURE DETECTION
 * ============================================================================ */

/* CPUID support and feature detection */
uint8_t x86_cpuid_supported(void);
void x86_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, 
                uint32_t* ecx, uint32_t* edx);
const char* x86_get_cpu_vendor(void);
uint32_t x86_get_cpu_features(void);
uint32_t x86_get_cpu_extended_features(void);

/* CPU feature flags (CPUID leaf 1, EDX) */
#define X86_FEATURE_FPU         (1 << 0)   /* Floating Point Unit */
#define X86_FEATURE_VME         (1 << 1)   /* Virtual Mode Extension */
#define X86_FEATURE_DE          (1 << 2)   /* Debugging Extension */
#define X86_FEATURE_PSE         (1 << 3)   /* Page Size Extension */
#define X86_FEATURE_TSC         (1 << 4)   /* Time Stamp Counter */
#define X86_FEATURE_MSR         (1 << 5)   /* Model Specific Registers */
#define X86_FEATURE_PAE         (1 << 6)   /* Physical Address Extension */
#define X86_FEATURE_MCE         (1 << 7)   /* Machine Check Exception */
#define X86_FEATURE_CX8         (1 << 8)   /* CMPXCHG8 Instruction */
#define X86_FEATURE_APIC        (1 << 9)   /* Advanced PIC */
#define X86_FEATURE_SEP         (1 << 11)  /* SYSENTER/SYSEXIT */
#define X86_FEATURE_MTRR        (1 << 12)  /* Memory Type Range Registers */
#define X86_FEATURE_PGE         (1 << 13)  /* Page Global Enable */
#define X86_FEATURE_MCA         (1 << 14)  /* Machine Check Architecture */
#define X86_FEATURE_CMOV        (1 << 15)  /* Conditional Move Instructions */
#define X86_FEATURE_PAT         (1 << 16)  /* Page Attribute Table */
#define X86_FEATURE_PSE36       (1 << 17)  /* 36-bit Page Size Extension */
#define X86_FEATURE_CLFLUSH     (1 << 19)  /* CLFLUSH Instruction */
#define X86_FEATURE_MMX         (1 << 23)  /* MMX Technology */
#define X86_FEATURE_FXSR        (1 << 24)  /* FXSAVE/FXRSTOR */
#define X86_FEATURE_SSE         (1 << 25)  /* SSE */
#define X86_FEATURE_SSE2        (1 << 26)  /* SSE2 */

/* ============================================================================
 * X86 VGA TEXT MODE FUNCTIONS
 * ============================================================================ */

/* VGA text mode operations */
meow_error_t x86_vga_init(void);
meow_error_t x86_vga_clear_screen(void);
meow_error_t x86_vga_set_color(uint8_t foreground, uint8_t background);
void x86_vga_putc(char c);
meow_error_t x86_vga_puts(const char* str);
meow_error_t x86_vga_set_cursor(uint8_t x, uint8_t y);
meow_error_t x86_vga_get_cursor(uint8_t* x, uint8_t* y);
meow_error_t x86_vga_scroll_up(void);

/* ============================================================================
 * X86 INTERRUPT SERVICE ROUTINES (ASSEMBLY STUBS)
 * ============================================================================ */

/* CPU Exception handlers (0-31) */
extern void x86_isr0(void);   /* Division by zero */
extern void x86_isr1(void);   /* Debug */
extern void x86_isr2(void);   /* Non-maskable interrupt */
extern void x86_isr3(void);   /* Breakpoint */
extern void x86_isr4(void);   /* Overflow */
extern void x86_isr5(void);   /* Bound range exceeded */
extern void x86_isr6(void);   /* Invalid opcode */
extern void x86_isr7(void);   /* Device not available */
extern void x86_isr8(void);   /* Double fault */
extern void x86_isr9(void);   /* Coprocessor segment overrun */
extern void x86_isr10(void);  /* Invalid TSS */
extern void x86_isr11(void);  /* Segment not present */
extern void x86_isr12(void);  /* Stack fault */
extern void x86_isr13(void);  /* General protection fault */
extern void x86_isr14(void);  /* Page fault */
extern void x86_isr15(void);  /* Reserved */
extern void x86_isr16(void);  /* Floating point exception */
extern void x86_isr17(void);  /* Alignment check */
extern void x86_isr18(void);  /* Machine check */
extern void x86_isr19(void);  /* SIMD floating point exception */
extern void x86_isr20(void);  /* Reserved */
extern void x86_isr21(void);  /* Reserved */
extern void x86_isr22(void);  /* Reserved */
extern void x86_isr23(void);  /* Reserved */
extern void x86_isr24(void);  /* Reserved */
extern void x86_isr25(void);  /* Reserved */
extern void x86_isr26(void);  /* Reserved */
extern void x86_isr27(void);  /* Reserved */
extern void x86_isr28(void);  /* Reserved */
extern void x86_isr29(void);  /* Reserved */
extern void x86_isr30(void);  /* Reserved */
extern void x86_isr31(void);  /* Reserved */

/* Hardware IRQ handlers (32-47) */
extern void x86_irq0(void);   /* Timer */
extern void x86_irq1(void);   /* Keyboard */
extern void x86_irq2(void);   /* Cascade */
extern void x86_irq3(void);   /* COM2/COM4 */
extern void x86_irq4(void);   /* COM1/COM3 */
extern void x86_irq5(void);   /* LPT2 */
extern void x86_irq6(void);   /* Floppy */
extern void x86_irq7(void);   /* LPT1 */
extern void x86_irq8(void);   /* RTC */
extern void x86_irq9(void);   /* Available */
extern void x86_irq10(void);  /* Available */
extern void x86_irq11(void);  /* Available */
extern void x86_irq12(void);  /* Mouse */
extern void x86_irq13(void);  /* Coprocessor */
extern void x86_irq14(void);  /* Primary ATA */
extern void x86_irq15(void);  /* Secondary ATA */

/* ============================================================================
 * X86 DEBUG AND DIAGNOSTIC FUNCTIONS
 * ============================================================================ */

/* System information and debugging */
meow_error_t x86_dump_gdt(void);
meow_error_t x86_dump_idt(void);
meow_error_t x86_dump_cpu_state(x86_cpu_state_t* state);
meow_error_t x86_dump_memory_map(void);
meow_error_t x86_run_cpu_tests(void);

/* Hardware testing functions */
meow_error_t x86_test_memory(uint32_t start, uint32_t size);
meow_error_t x86_test_interrupts(void);
meow_error_t x86_test_timer(void);

/* ============================================================================
 * X86 UTILITY MACROS AND INLINE FUNCTIONS
 * ============================================================================ */

/* Alignment macros */
#define X86_ALIGN_DOWN(addr, align)     ((addr) & ~((align) - 1))
#define X86_ALIGN_UP(addr, align)       (((addr) + (align) - 1) & ~((align) - 1))
#define X86_IS_ALIGNED(addr, align)     (((addr) & ((align) - 1)) == 0)

/* Page manipulation macros */
#define X86_PAGE_FRAME(addr)            ((addr) & X86_PAGE_ALIGN_MASK)
#define X86_PAGE_OFFSET(addr)           ((addr) & (X86_PAGE_SIZE - 1))
#define X86_PAGES_FOR_SIZE(size)        (((size) + X86_PAGE_SIZE - 1) / X86_PAGE_SIZE)

/* Bit manipulation macros */
#define X86_BIT_SET(value, bit)         ((value) |= (1 << (bit)))
#define X86_BIT_CLEAR(value, bit)       ((value) &= ~(1 << (bit)))
#define X86_BIT_TEST(value, bit)        (((value) & (1 << (bit))) != 0)
#define X86_BIT_FLIP(value, bit)        ((value) ^= (1 << (bit)))

/* Error handling macros specific to x86 */
#define X86_RETURN_IF_ERROR(expr) do { \
    meow_error_t _err = (expr); \
    if (_err != MEOW_SUCCESS) { \
        meow_error("x86: Operation failed at %s:%d - error %d", \
                   __FILE__, __LINE__, _err); \
        return _err; \
    } \
} while(0)

#define X86_VALIDATE_IRQ(irq) do { \
    if ((irq) >= 16) { \
        meow_error("x86: Invalid IRQ number: %u", irq); \
        return MEOW_ERROR_INVALID_PARAMETER; \
    } \
} while(0)

/* ============================================================================
 * X86 FORWARD DECLARATIONS
 * ============================================================================ */

/* Timer tick callback (called from interrupt handler) */
void x86_timer_tick(void);

/* Common interrupt handler (called from assembly stubs) */
void x86_common_interrupt_handler(x86_cpu_state_t* state);

/* Exception handlers */
void x86_handle_division_by_zero(x86_cpu_state_t* state);
void x86_handle_page_fault(x86_cpu_state_t* state);
void x86_handle_general_protection_fault(x86_cpu_state_t* state);
void x86_handle_double_fault(x86_cpu_state_t* state);

#endif /* MEOW_HAL_X86_H */