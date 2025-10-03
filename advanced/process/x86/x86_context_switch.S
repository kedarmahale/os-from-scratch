/* advanced/process/x86/x86_context_switch.S - x86 Context Switching
 *
 * Low-level assembly for task context switching on x86
 * Copyright (c) 2025 MeowKernel Project
 */

.section .text
.global task_switch_context
.global get_current_esp
.global set_current_esp

/* void task_switch_context(task_t* from, task_t* to) */
task_switch_context:
    /* Function parameters:
     * [ebp+8]  = from task pointer
     * [ebp+12] = to task pointer
     */
    
    push %ebp
    mov %esp, %ebp
    
    /* Save current task's context if from != NULL */
    mov 8(%ebp), %eax        /* Load 'from' task pointer */
    test %eax, %eax          /* Check if from is NULL */
    jz restore_context       /* If NULL, skip saving */
    
    /* Save general purpose registers */
    push %edi
    push %esi
    push %ebx
    push %ecx
    push %edx
    
    /* Save segment registers */
    push %ds
    push %es
    push %fs
    push %gs
    
    /* Save current ESP in task's context structure */
    mov %esp, %ebx           /* Current ESP */
    mov 64(%eax), %ecx       /* Get context pointer from task structure */
    mov %ebx, 28(%ecx)       /* Save ESP in context->esp */
    
    /* Save other registers in context structure */
    mov %edi, 0(%ecx)        /* context->edi */
    mov %esi, 4(%ecx)        /* context->esi */
    mov %ebp, 8(%ecx)        /* context->ebp */
    mov %ebx, 16(%ecx)       /* context->ebx */
    mov %edx, 20(%ecx)       /* context->edx */
    pop %edx                 /* Get ECX from stack */
    mov %edx, 24(%ecx)       /* context->ecx */
    pop %edx                 /* Get EAX from stack */
    mov %edx, 28(%ecx)       /* context->eax */

restore_context:
    /* Load new task's context */
    mov 12(%ebp), %eax       /* Load 'to' task pointer */
    test %eax, %eax          /* Check if to is NULL */
    jz context_switch_done   /* If NULL, we have a problem */
    
    mov 64(%eax), %ecx       /* Get context pointer from task structure */
    
    /* Restore ESP first */
    mov 28(%ecx), %esp       /* Restore ESP from context */
    
    /* Restore segment registers */
    pop %gs
    pop %fs
    pop %es
    pop %ds
    
    /* Restore general purpose registers */
    pop %edx
    pop %ecx
    pop %ebx
    pop %esi
    pop %edi
    
    /* Update current task pointer (extern variable) */
    mov %eax, current_task

context_switch_done:
    pop %ebp
    ret

/* uint32_t get_current_esp(void) - Get current stack pointer */
get_current_esp:
    mov %esp, %eax
    ret

/* void set_current_esp(uint32_t esp) - Set current stack pointer */
set_current_esp:
    mov 4(%esp), %eax
    mov %eax, %esp
    ret

/* Task entry point wrapper - called when task starts */
.global task_entry_wrapper
task_entry_wrapper:
    /* At this point, the stack should have:
     * [esp+4] = task argument
     * [esp+0] = return address (should never be used)
     */
    
    /* Enable interrupts for new task */
    sti
    
    /* Get argument and call task entry point */
    pop %eax                 /* Remove return address */
    pop %eax                 /* Get task argument */
    push %eax                /* Push as parameter */
    
    /* Call the actual task entry point */
    /* This address should be set up by the task creation code */
    call *%ebx               /* Entry point should be in EBX */
    
    /* If task returns, terminate it */
    push $0                  /* Exit code 0 */
    call task_terminate_current
    
    /* Should never reach here */
    cli
    hlt

/* void task_terminate_current(int exit_code) */
.global task_terminate_current
task_terminate_current:
    /* Get exit code from stack */
    mov 4(%esp), %eax
    
    /* Call C function to terminate current task */
    push %eax
    call task_terminate_self
    add $4, %esp
    
    /* Should never return, but just in case */
    cli
    hlt

/* Save all registers for interrupt context */
.global save_interrupt_context
save_interrupt_context:
    pusha                    /* Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI */
    push %ds
    push %es
    push %fs
    push %gs
    
    /* Load kernel data segment */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    ret

/* Restore all registers from interrupt context */
.global restore_interrupt_context
restore_interrupt_context:
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa                     /* Restore all general purpose registers */
    ret

/* Switch to user mode (for future user space support) */
.global switch_to_user_mode
switch_to_user_mode:
    /* This will be implemented when we add user space support */
    /* For now, all tasks run in kernel mode */
    ret

/* Get current task pointer */
.global get_current_task_asm
get_current_task_asm:
    mov current_task, %eax
    ret

/* Set current task pointer */
.global set_current_task_asm
set_current_task_asm:
    mov 4(%esp), %eax
    mov %eax, current_task
    ret