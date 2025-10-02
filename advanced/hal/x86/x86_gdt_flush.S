/* advanced/hal/x86/gdt_flush.S - GDT loading assembly code */

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    /* Load GDT pointer from parameter */
    movl 4(%esp), %eax
    lgdt (%eax)
    
    /* Reload segment registers */
    movl $0x10, %eax    # 0x10 is offset to kernel data segment
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    /* Far jump to reload CS (kernel code segment) */
    ljmp $0x08, $.reload_cs  # 0x08 is offset to kernel code segment

.reload_cs:
    ret

.size gdt_flush, . - gdt_flush
