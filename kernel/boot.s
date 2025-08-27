# boot.s - Bootstrap assembly for multiboot kernel
# This sets up the multiboot header and stack, then calls the C kernel

.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a multiboot header that marks the program as a kernel
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate a stack for our kernel (16KB)
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# The kernel entry point
.section .text
.global _start
.type _start, @function
_start:
    # Set up the stack pointer to the top of our stack
    # (Remember: the stack grows downward on x86)
    mov $stack_top, %esp

    # Reset EFLAGS - clear direction flag and interrupt flag
    pushl $0
    popf

    # Call the main kernel function written in C
    call kernel_main

    # If kernel_main returns (it shouldn't), put the CPU into infinite loop
    cli          # Disable interrupts
1:  hlt          # Halt the CPU
    jmp 1b       # Jump back to halt instruction

# Set the size of the _start symbol to current location '.' minus its start
.size _start, . - _start
