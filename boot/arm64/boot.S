/* boot/arm64/boot.S - ARM64 boot code for Raspberry Pi */

.section ".text.boot"
.global _start

_start:
    /* Get current CPU ID */
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, master_cpu
    
    /* Secondary CPUs go to sleep */
secondary_sleep:
    wfe
    b secondary_sleep

master_cpu:
    /* Set stack pointer */
    ldr x1, =stack_top
    mov sp, x1
    
    /* Clear BSS section */
    ldr x1, =_bss_start
    ldr x2, =_bss_end
    
clear_bss:
    cmp x1, x2
    b.ge call_kernel
    str xzr, [x1], #8
    b clear_bss
    
call_kernel:
    /* Enable floating point */
    mrs x0, cpacr_el1
    orr x0, x0, #(3 << 20)
    msr cpacr_el1, x0
    
    /* Call kernel main */
    bl kernel_main
    
    /* Should never return, but just in case... */
hang:
    wfe
    b hang

.section ".bss"
.align 4
stack_bottom:
.space 65536  /* 64KB stack */
stack_top:

/* BSS markers */
_bss_start:
_bss_end:
