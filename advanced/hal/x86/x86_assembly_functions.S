# advanced/hal/x86/x86_stubs.S - Missing x86 Assembly Functions
# 
# Assembly stubs for low-level x86 operations
# Copyright (c) 2025 MeowKernel Project

.section .text

# ============================================================================
# GDT Functions
# ============================================================================

.global x86_gdt_flush
.type x86_gdt_flush, @function

# void x86_gdt_flush(uint32_t gdt_ptr)
x86_gdt_flush:
    movl 4(%esp), %eax      # Get GDT pointer from stack
    lgdt (%eax)             # Load GDT
    
    # Reload segment registers with new GDT
    movw $0x10, %ax         # Kernel data segment (GDT entry 2)
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    # Far jump to reload CS (code segment)
    ljmp $0x08, $gdt_flush_complete  # Kernel code segment (GDT entry 1)
    
gdt_flush_complete:
    ret

# ============================================================================
# IDT Functions  
# ============================================================================

.global x86_idt_flush
.type x86_idt_flush, @function

# void x86_idt_flush(uint32_t idt_ptr)
x86_idt_flush:
    movl 4(%esp), %eax      # Get IDT pointer from stack
    lidt (%eax)             # Load IDT
    ret

# ============================================================================
# CPUID Functions
# ============================================================================

.global x86_cpuid_supported
.type x86_cpuid_supported, @function

# uint32_t x86_cpuid_supported(void)
x86_cpuid_supported:
    pushf                   # Save flags
    pushf                   # Save flags again
    
    xorl $0x200000, (%esp)  # Flip ID bit (bit 21) in stacked flags
    popf                    # Load modified flags
    
    pushf                   # Get flags back
    popl %eax               # Flags to EAX
    xorl (%esp), %eax       # XOR with original flags
    popf                    # Restore original flags
    
    andl $0x200000, %eax    # Isolate ID bit
    shrl $21, %eax          # Shift to get 0 or 1
    ret

.global x86_cpuid
.type x86_cpuid, @function

# void x86_cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
x86_cpuid:
    pushl %ebx              # Save EBX (required by calling convention)
    pushl %ecx              # Save ECX  
    pushl %edx              # Save EDX
    
    movl 16(%esp), %eax     # Get CPUID function code from stack
    cpuid                   # Execute CPUID
    
    # Store results in output parameters
    movl 20(%esp), %edi     # Get eax pointer
    movl %eax, (%edi)       # Store EAX result
    
    movl 24(%esp), %edi     # Get ebx pointer  
    movl %ebx, (%edi)       # Store EBX result
    
    movl 28(%esp), %edi     # Get ecx pointer
    movl %ecx, (%edi)       # Store ECX result
    
    movl 32(%esp), %edi     # Get edx pointer
    movl %edx, (%edi)       # Store EDX result
    
    popl %edx               # Restore EDX
    popl %ecx               # Restore ECX  
    popl %ebx               # Restore EBX
    ret

# ============================================================================
# I/O Port Functions
# ============================================================================

.global x86_outb
.type x86_outb, @function

# void x86_outb(uint16_t port, uint8_t value)
x86_outb:
    movl 4(%esp), %edx      # Port number to DX
    movl 8(%esp), %eax      # Value to AL
    outb %al, %dx           # Output byte
    ret

.global x86_inb
.type x86_inb, @function

# uint8_t x86_inb(uint16_t port)  
x86_inb:
    movl 4(%esp), %edx      # Port number to DX
    inb %dx, %al            # Input byte to AL
    movzbl %al, %eax        # Zero-extend AL to EAX
    ret

.global x86_outw
.type x86_outw, @function

# void x86_outw(uint16_t port, uint16_t value)
x86_outw:
    movl 4(%esp), %edx      # Port number to DX
    movl 8(%esp), %eax      # Value to AX
    outw %ax, %dx           # Output word
    ret

.global x86_inw
.type x86_inw, @function

# uint16_t x86_inw(uint16_t port)
x86_inw:
    movl 4(%esp), %edx      # Port number to DX
    inw %dx, %ax            # Input word to AX
    movzwl %ax, %eax        # Zero-extend AX to EAX
    ret

.global x86_outl
.type x86_outl, @function

# void x86_outl(uint16_t port, uint32_t value)  
x86_outl:
    movl 4(%esp), %edx      # Port number to DX
    movl 8(%esp), %eax      # Value to EAX
    outl %eax, %dx          # Output long
    ret

.global x86_inl
.type x86_inl, @function

# uint32_t x86_inl(uint16_t port)
x86_inl:
    movl 4(%esp), %edx      # Port number to DX
    inl %dx, %eax           # Input long to EAX
    ret

# ============================================================================
# CPU Control Functions
# ============================================================================

.global x86_cli
.type x86_cli, @function

# void x86_cli(void) - Disable interrupts
x86_cli:
    cli
    ret

.global x86_sti  
.type x86_sti, @function

# void x86_sti(void) - Enable interrupts
x86_sti:
    sti
    ret

.global x86_hlt
.type x86_hlt, @function

# void x86_hlt(void) - Halt processor
x86_hlt:
    hlt
    ret

.global x86_io_wait
.type x86_io_wait, @function

# void x86_io_wait(void) - Short I/O delay
x86_io_wait:
    outb %al, $0x80         # Write to unused port for delay
    ret

# ============================================================================
# Control Register Functions  
# ============================================================================

.global x86_get_cr0
.type x86_get_cr0, @function

# uint32_t x86_get_cr0(void)
x86_get_cr0:
    movl %cr0, %eax
    ret

.global x86_get_cr2
.type x86_get_cr2, @function

# uint32_t x86_get_cr2(void)
x86_get_cr2:
    movl %cr2, %eax  
    ret

.global x86_get_cr3
.type x86_get_cr3, @function

# uint32_t x86_get_cr3(void)
x86_get_cr3:
    movl %cr3, %eax
    ret

.global x86_get_eflags
.type x86_get_eflags, @function

# uint32_t x86_get_eflags(void)
x86_get_eflags:
    pushf
    popl %eax
    ret

# End of file