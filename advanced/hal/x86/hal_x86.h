/* advanced/hal/x86/hal_x86.h - Enhanced x86 HAL interface */

#ifndef HAL_X86_H
#define HAL_X86_H

#include "../hal.h"
#include "../../mm/territory_map.h"

/* Memory detection and management */
uint32_t detect_memory_from_multiboot(multiboot_info_t* mbi);
uint32_t get_kernel_memory_usage(void);
void hal_set_multiboot_info(multiboot_info_t* mbi);

/* Interrupt management */
void (*hal_interrupt_get_handler(uint8_t interrupt))(void);
void hal_interrupt_unregister_handler(uint8_t interrupt);

/* Enhanced debug output */
void hal_debug_printf(const char* format, ...);

/* x86 control register operations */
void x86_set_cr0(uint32_t cr0);
void x86_set_cr3(uint32_t cr3);

/* CPU feature detection */
uint32_t x86_cpuid_supported(void);
void x86_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);
void x86_get_cpu_vendor(char* vendor);

/* HAL diagnostics and testing */
void hal_show_system_info(void);
uint8_t hal_self_test(void);
uint8_t hal_is_initialized(void);
void hal_set_initialized(void);
void hal_emergency_halt(const char* reason);

/* x86-specific initialization functions */
void x86_gdt_init(void);
void x86_idt_init(void);
void x86_pic_init(void);
void x86_pit_init(uint32_t frequency);

/* PIC functions */
void x86_pic_eoi(uint8_t irq);
void x86_pic_enable_irq(uint8_t irq);
void x86_pic_disable_irq(uint8_t irq);

/* Timer functions */
uint32_t x86_pit_get_frequency(void);
void x86_pit_sleep(uint32_t milliseconds);

/* Interrupt handling */
void interrupt_handler(uint32_t interrupt_number, uint32_t error_code);

/* Assembly helper functions */
extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void idt_flush(uint32_t idt_ptr_addr);

/* Inline assembly helpers */
static inline void x86_cli(void) {
    asm volatile("cli");
}

static inline void x86_sti(void) {
    asm volatile("sti");
}

static inline void x86_hlt(void) {
    asm volatile("hlt");
}

static inline uint32_t x86_get_eflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

/* Interrupt acknowledgment */
static inline void x86_irq_ack(uint8_t irq) {
    x86_pic_eoi(irq);
}

#endif /* HAL_X86_H */
