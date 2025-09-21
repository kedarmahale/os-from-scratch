/* advanced/hal/x86/interrupts.S - Interrupt handler assembly code */

.section .text

/* Common interrupt stub that calls C handler */
interrupt_common_stub:
    /* Save all general-purpose registers */
    pusha
    
    /* Save segment selectors */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    /* Load kernel data segment */
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    /* Call C interrupt handler */
    call interrupt_handler
    
    /* Restore segment selectors */
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    /* Restore all general-purpose registers */
    popa
    
    /* Remove error code and interrupt number from stack */
    addl $8, %esp
    
    /* Return from interrupt */
    iret

/* Macro for interrupt service routines WITHOUT error code */
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    pushl $0        /* Push dummy error code */
    pushl $\num     /* Push interrupt number */
    jmp interrupt_common_stub
.endm

/* Macro for interrupt service routines WITH error code */
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    pushl $\num     /* Push interrupt number (error code already on stack) */
    jmp interrupt_common_stub
.endm

/* Macro for IRQ handlers */
.macro IRQ num, irq_num
.global irq\num
irq\num:
    cli
    pushl $0        /* Push dummy error code */
    pushl $\irq_num /* Push IRQ number (32 + num) */
    jmp interrupt_common_stub
.endm

/* CPU Exception handlers (0-31) */
ISR_NOERRCODE 0     /* Division by zero */
ISR_NOERRCODE 1     /* Debug */
ISR_NOERRCODE 2     /* Non-maskable interrupt */
ISR_NOERRCODE 3     /* Breakpoint */
ISR_NOERRCODE 4     /* Overflow */
ISR_NOERRCODE 5     /* Bound range exceeded */
ISR_NOERRCODE 6     /* Invalid opcode */
ISR_NOERRCODE 7     /* Device not available */
ISR_ERRCODE   8     /* Double fault */
ISR_NOERRCODE 9     /* Coprocessor segment overrun */
ISR_ERRCODE   10    /* Invalid TSS */
ISR_ERRCODE   11    /* Segment not present */
ISR_ERRCODE   12    /* Stack fault */
ISR_ERRCODE   13    /* General protection fault */
ISR_ERRCODE   14    /* Page fault */
ISR_NOERRCODE 15    /* Reserved */
ISR_NOERRCODE 16    /* x87 floating point exception */
ISR_ERRCODE   17    /* Alignment check */
ISR_NOERRCODE 18    /* Machine check */
ISR_NOERRCODE 19    /* SIMD floating point exception */
ISR_NOERRCODE 20    /* Virtualization exception */
ISR_NOERRCODE 21    /* Reserved */
ISR_NOERRCODE 22    /* Reserved */
ISR_NOERRCODE 23    /* Reserved */
ISR_NOERRCODE 24    /* Reserved */
ISR_NOERRCODE 25    /* Reserved */
ISR_NOERRCODE 26    /* Reserved */
ISR_NOERRCODE 27    /* Reserved */
ISR_NOERRCODE 28    /* Reserved */
ISR_NOERRCODE 29    /* Reserved */
ISR_NOERRCODE 30    /* Reserved */
ISR_NOERRCODE 31    /* Reserved */

/* Hardware IRQ handlers (32-47) */
IRQ 0, 32    /* Timer */
IRQ 1, 33    /* Keyboard */
IRQ 2, 34    /* Cascade (never raised) */
IRQ 3, 35    /* Serial port 2 */
IRQ 4, 36    /* Serial port 1 */
IRQ 5, 37    /* Parallel port 2 / Sound card */
IRQ 6, 38    /* Floppy disk */
IRQ 7, 39    /* Parallel port 1 / Sound card */
IRQ 8, 40    /* Real-time clock */
IRQ 9, 41    /* ACPI */
IRQ 10, 42   /* Available / Network / Sound */
IRQ 11, 43   /* Available / USB / Sound */
IRQ 12, 44   /* PS/2 mouse */
IRQ 13, 45   /* FPU / Coprocessor / Inter-processor */
IRQ 14, 46   /* Primary ATA hard disk */
IRQ 15, 47   /* Secondary ATA hard disk */

/* IDT flush function */
.global idt_flush
.type idt_flush, @function
idt_flush:
    movl 4(%esp), %eax
    lidt (%eax)
    ret

.size idt_flush, . - idt_flush
