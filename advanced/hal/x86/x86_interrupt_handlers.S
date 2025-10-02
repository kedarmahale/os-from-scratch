/* advanced/hal/x86/interrupts.S - Interrupt Handler Stubs
 *
 * Assembly interrupt handler stubs for x86 architecture
 * Copyright (c) 2025 MeowKernel Project  
 */

.section .text

# ============================================================================
# INTERRUPT DESCRIPTOR TABLE ENTRIES
# ============================================================================

# Exception handlers (ISR 0-31)
.global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
.global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
.global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
.global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

# Hardware interrupt handlers (IRQ 0-15)
.global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
.global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

# ============================================================================
# EXCEPTION HANDLERS (ISR 0-31)
# ============================================================================

# Exceptions without error codes
isr0:   # Division by Zero
    push $0         # Dummy error code
    push $0         # Interrupt number
    jmp isr_common_stub

isr1:   # Debug
    push $0
    push $1
    jmp isr_common_stub

isr2:   # Non-maskable Interrupt
    push $0
    push $2
    jmp isr_common_stub

isr3:   # Breakpoint
    push $0
    push $3
    jmp isr_common_stub

isr4:   # Overflow
    push $0
    push $4
    jmp isr_common_stub

isr5:   # Bound Range Exceeded
    push $0
    push $5
    jmp isr_common_stub

isr6:   # Invalid Opcode
    push $0
    push $6
    jmp isr_common_stub

isr7:   # Device Not Available
    push $0
    push $7
    jmp isr_common_stub

# Exceptions with error codes (don't push dummy error code)
isr8:   # Double Fault
    push $8         # Interrupt number (error code already pushed by CPU)
    jmp isr_common_stub

isr9:   # Coprocessor Segment Overrun
    push $0
    push $9
    jmp isr_common_stub

isr10:  # Invalid TSS
    push $10
    jmp isr_common_stub

isr11:  # Segment Not Present
    push $11
    jmp isr_common_stub

isr12:  # Stack Fault
    push $12
    jmp isr_common_stub

isr13:  # General Protection Fault
    push $13
    jmp isr_common_stub

isr14:  # Page Fault
    push $14
    jmp isr_common_stub

isr15:  # Reserved
    push $0
    push $15
    jmp isr_common_stub

isr16:  # x87 FPU Error
    push $0
    push $16
    jmp isr_common_stub

isr17:  # Alignment Check
    push $17
    jmp isr_common_stub

isr18:  # Machine Check
    push $0
    push $18
    jmp isr_common_stub

isr19:  # SIMD Floating Point Exception
    push $0
    push $19
    jmp isr_common_stub

# Reserved exceptions (20-31)
isr20:
    push $0
    push $20
    jmp isr_common_stub

isr21:
    push $0
    push $21
    jmp isr_common_stub

isr22:
    push $0
    push $22
    jmp isr_common_stub

isr23:
    push $0
    push $23
    jmp isr_common_stub

isr24:
    push $0
    push $24
    jmp isr_common_stub

isr25:
    push $0
    push $25
    jmp isr_common_stub

isr26:
    push $0
    push $26
    jmp isr_common_stub

isr27:
    push $0
    push $27
    jmp isr_common_stub

isr28:
    push $0
    push $28
    jmp isr_common_stub

isr29:
    push $0
    push $29
    jmp isr_common_stub

isr30:
    push $0
    push $30
    jmp isr_common_stub

isr31:
    push $0
    push $31
    jmp isr_common_stub

# ============================================================================
# HARDWARE INTERRUPT HANDLERS (IRQ 0-15)  
# ============================================================================

# Master PIC (IRQ 0-7)
irq0:   # Timer
    push $0
    push $32        # IRQ 0 maps to interrupt 32
    jmp interrupt_common_stub

irq1:   # Keyboard
    push $0
    push $33        # IRQ 1 maps to interrupt 33
    jmp interrupt_common_stub

irq2:   # Cascade (never raised)
    push $0
    push $34
    jmp interrupt_common_stub

irq3:   # Serial port 2
    push $0
    push $35
    jmp interrupt_common_stub

irq4:   # Serial port 1
    push $0
    push $36
    jmp interrupt_common_stub

irq5:   # Parallel port 2
    push $0
    push $37
    jmp interrupt_common_stub

irq6:   # Floppy disk
    push $0
    push $38
    jmp interrupt_common_stub

irq7:   # Parallel port 1
    push $0
    push $39
    jmp interrupt_common_stub

# Slave PIC (IRQ 8-15)
irq8:   # Real-time clock
    push $0
    push $40        # IRQ 8 maps to interrupt 40
    jmp interrupt_common_stub

irq9:   # ACPI
    push $0
    push $41
    jmp interrupt_common_stub

irq10:  # Available
    push $0
    push $42
    jmp interrupt_common_stub

irq11:  # Available
    push $0
    push $43
    jmp interrupt_common_stub

irq12:  # PS/2 mouse
    push $0
    push $44
    jmp interrupt_common_stub

irq13:  # FPU
    push $0
    push $45
    jmp interrupt_common_stub

irq14:  # Primary ATA
    push $0
    push $46
    jmp interrupt_common_stub

irq15:  # Secondary ATA
    push $0
    push $47
    jmp interrupt_common_stub

# ============================================================================
# COMMON INTERRUPT STUBS
# ============================================================================

# Common stub for CPU exceptions
isr_common_stub:
    pusha               # Push all general-purpose registers
    
    push %ds            # Push data segments
    push %es
    push %fs
    push %gs
    
    mov $0x10, %ax      # Load kernel data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    push %esp           # Pass stack pointer to C handler
    call x86_idt_handle_interrupt   # Call C exception handler
    add $4, %esp        # Clean up stack parameter
    
    pop %gs             # Restore segments
    pop %fs
    pop %es
    pop %ds
    
    popa                # Restore all general-purpose registers
    add $8, %esp        # Remove error code and interrupt number
    iret                # Return from interrupt

# Common stub for hardware interrupts  
interrupt_common_stub:
    pusha               # Push all general-purpose registers
    
    push %ds            # Push data segments
    push %es
    push %fs
    push %gs
    
    mov $0x10, %ax      # Load kernel data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    call interrupt_handler  # Call C interrupt handler
    
    pop %gs             # Restore segments
    pop %fs
    pop %es
    pop %ds
    
    popa                # Restore all general-purpose registers
    add $8, %esp        # Remove error code and interrupt number
    iret                # Return from interrupt

# End of file