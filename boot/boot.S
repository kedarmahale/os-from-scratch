/* boot.S - Kernel entry point using GAS/AT&T syntax */

/* Multiboot header constants */
.set ALIGN,     1<<0
.set MEMINFO,   1<<1
.set FLAGS,     ALIGN | MEMINFO
.set MAGIC,     0x1BADB002
.set CHECKSUM,  -(MAGIC + FLAGS)

/* Multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack for initial kernel thread */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Entry point */
.section .text
.global _start
.type _start, @function

_start:
    /* Set up the stack - AT&T syntax: movl source, destination */
    movl $stack_top, %esp
    
    /* Reset EFLAGS */
    pushl $0
    popf
    
    /* Call the main kernel function */
    call kernel_main
    
    /* If kernel_main returns, halt the system */
    cli
1:  hlt
    jmp 1b

/* Set the size of the _start symbol */
.size _start, . - _start

