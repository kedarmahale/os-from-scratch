/* Architecture-independent HAL interface */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stddef.h>

/* Define our own boolean type */
typedef int bool;
#define true 1
#define false 0

/* HAL Initialization */
void hal_init(void);

/* CPU Operations */
void hal_cpu_init(void);
void hal_cpu_halt(void);
void hal_cpu_disable_interrupts(void);
void hal_cpu_enable_interrupts(void);
uint32_t hal_cpu_get_flags(void);

/* Port I/O Operations (x86 specific, stubbed on ARM) */
uint8_t hal_port_inb(uint16_t port);
uint16_t hal_port_inw(uint16_t port);
uint32_t hal_port_inl(uint16_t port);
void hal_port_outb(uint16_t port, uint8_t data);
void hal_port_outw(uint16_t port, uint16_t data);
void hal_port_outl(uint16_t port, uint32_t data);

/* Memory Operations */
void hal_memory_init(void);
uint32_t hal_memory_get_total_size(void);
uint32_t hal_memory_get_available_size(void);
void* hal_memory_get_kernel_end(void);

/* Timer Operations */
void hal_timer_init(uint32_t frequency);
uint64_t hal_timer_get_ticks(void);
void hal_timer_sleep(uint32_t milliseconds);
void hal_timer_tick(void);

/* Interrupt Operations */
void hal_interrupt_init(void);
void hal_interrupt_register_handler(uint8_t interrupt, void (*handler)(void));
void hal_interrupt_enable(uint8_t interrupt);
void hal_interrupt_disable(uint8_t interrupt);

/* Debug Output */
void hal_debug_putc(char c);
void hal_debug_puts(const char* str);

/* Architecture Detection */
typedef enum  {
    ARCH_X86,
    ARCH_X86_64,
    ARCH_ARM,
    ARCH_AARCH64,
    ARCH_UNKNOWN
} hal_arch_t;

hal_arch_t hal_get_architecture(void);
const char* hal_get_arch_string(void);

#endif /* HAL_H */

