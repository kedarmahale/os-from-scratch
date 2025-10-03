/* advanced/syscalls/x86/x86_syscall_entry.S - x86 System Call Entry Point
 *
 * Assembly handler for system call interrupt (int 0x80)
 * Copyright (c) 2025 MeowKernel Project
 */

.section .text
.global syscall_interrupt_handler
.global syscall_entry_point

/* System call interrupt handler (called by int 0x80) */
syscall_interrupt_handler:
    /* Save all registers */
    pusha                    /* Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI */
    push %ds
    push %es
    push %fs
    push %gs
    
    /* Load kernel data segments */
    mov $0x10, %ax          /* Kernel data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    /* System call parameters are in registers:
     * EAX = system call number
     * EBX = arg1
     * ECX = arg2  
     * EDX = arg3
     * ESI = arg4
     */
    
    /* Push parameters for C function call */
    push %esi               /* arg4 */
    push %edx               /* arg3 */
    push %ecx               /* arg2 */
    push %ebx               /* arg1 */
    push %eax               /* syscall_num */
    
    /* Call C system call dispatcher */
    call syscall_dispatcher
    
    /* Clean up stack (5 parameters * 4 bytes = 20 bytes) */
    add $20, %esp
    
    /* Return value is in EAX, save it */
    mov %eax, 28(%esp)      /* Store in saved EAX position on stack */
    
    /* Restore all registers */
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa                    /* Restore all general purpose registers */
    
    /* Return from interrupt */
    iret

/* Fast system call entry (for future optimization) */
syscall_entry_point:
    /* This could be used for faster system calls using SYSCALL/SYSRET */
    /* For now, just use the interrupt method */
    int $0x80
    ret

/* System call stub generator macro */
.macro SYSCALL_STUB name, number
.global \name
\name:
    mov $\number, %eax      /* Set system call number */
    int $0x80               /* Trigger system call interrupt */
    ret                     /* Return with result in EAX */
.endm

/* Generate system call stubs */
SYSCALL_STUB sys_exit_stub, 0
SYSCALL_STUB sys_write_stub, 1
SYSCALL_STUB sys_read_stub, 2
SYSCALL_STUB sys_open_stub, 3
SYSCALL_STUB sys_close_stub, 4
SYSCALL_STUB sys_fork_stub, 5
SYSCALL_STUB sys_getpid_stub, 6
SYSCALL_STUB sys_getppid_stub, 7
SYSCALL_STUB sys_wait_stub, 8
SYSCALL_STUB sys_exec_stub, 9
SYSCALL_STUB sys_brk_stub, 10
SYSCALL_STUB sys_mmap_stub, 11
SYSCALL_STUB sys_munmap_stub, 12
SYSCALL_STUB sys_sleep_stub, 13
SYSCALL_STUB sys_yield_stub, 14
SYSCALL_STUB sys_time_stub, 15
SYSCALL_STUB sys_meow_log_stub, 16
SYSCALL_STUB sys_get_stats_stub, 17

/* Cat-themed system call stubs */
SYSCALL_STUB sys_purr_stub, 100
SYSCALL_STUB sys_hiss_stub, 101
SYSCALL_STUB sys_chirp_stub, 102