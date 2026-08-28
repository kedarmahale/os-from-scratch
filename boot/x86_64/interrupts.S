.section .text
.code64
.global x86_64_load_idt
.type x86_64_load_idt, @function
x86_64_load_idt:
    lidt (%rdi)
    ret

.macro EXCEPTION_NO_ERROR vector
.global x86_64_exception_stub\vector
.type x86_64_exception_stub\vector, @function
x86_64_exception_stub\vector:
    pushq $0
    pushq $\vector
    jmp x86_64_exception_common
.endm

.macro EXCEPTION_WITH_ERROR vector
.global x86_64_exception_stub\vector
.type x86_64_exception_stub\vector, @function
x86_64_exception_stub\vector:
    pushq $\vector
    jmp x86_64_exception_common
.endm

.macro IRQ_STUB irq
.global x86_64_irq_stub\irq
.type x86_64_irq_stub\irq, @function
x86_64_irq_stub\irq:
    pushq $0
    pushq $(32 + \irq)
    jmp x86_64_irq_common
.endm

# CPU exceptions 8, 10-14, 17, 21, 29, and 30 provide an error code.
EXCEPTION_NO_ERROR 0
EXCEPTION_NO_ERROR 1
EXCEPTION_NO_ERROR 2
EXCEPTION_NO_ERROR 3
EXCEPTION_NO_ERROR 4
EXCEPTION_NO_ERROR 5
EXCEPTION_NO_ERROR 6
EXCEPTION_NO_ERROR 7
EXCEPTION_WITH_ERROR 8
EXCEPTION_NO_ERROR 9
EXCEPTION_WITH_ERROR 10
EXCEPTION_WITH_ERROR 11
EXCEPTION_WITH_ERROR 12
EXCEPTION_WITH_ERROR 13
EXCEPTION_WITH_ERROR 14
EXCEPTION_NO_ERROR 15
EXCEPTION_NO_ERROR 16
EXCEPTION_WITH_ERROR 17
EXCEPTION_NO_ERROR 18
EXCEPTION_NO_ERROR 19
EXCEPTION_NO_ERROR 20
EXCEPTION_WITH_ERROR 21
EXCEPTION_NO_ERROR 22
EXCEPTION_NO_ERROR 23
EXCEPTION_NO_ERROR 24
EXCEPTION_NO_ERROR 25
EXCEPTION_NO_ERROR 26
EXCEPTION_NO_ERROR 27
EXCEPTION_NO_ERROR 28
EXCEPTION_WITH_ERROR 29
EXCEPTION_WITH_ERROR 30
EXCEPTION_NO_ERROR 31

IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 2
IRQ_STUB 3
IRQ_STUB 4
IRQ_STUB 5
IRQ_STUB 6
IRQ_STUB 7
IRQ_STUB 8
IRQ_STUB 9
IRQ_STUB 10
IRQ_STUB 11
IRQ_STUB 12
IRQ_STUB 13
IRQ_STUB 14
IRQ_STUB 15

.global x86_64_unexpected_stub
.type x86_64_unexpected_stub, @function
x86_64_unexpected_stub:
    pushq $0
    pushq $255

x86_64_exception_common:
    cli
    cld
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    mov %rsp, %rdi
    mov %rsp, %rbp
    and $-16, %rsp
    call x86_64_exception_handler

    mov %rbp, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    add $16, %rsp
    iretq

x86_64_irq_common:
    cli
    cld
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    mov %rsp, %rdi
    mov %rsp, %rbp
    and $-16, %rsp
    call x86_64_irq_dispatch

    # The scheduler may return a frame on another task's stack.  Fall back to
    # the frame that interrupted the current task when no switch was needed.
    test %rax, %rax
    jnz 1f
    mov %rbp, %rax
1:
    mov %rax, %rdi
    jmp x86_64_restore_irq_frame

.global x86_64_restore_irq_frame
.type x86_64_restore_irq_frame, @function
x86_64_restore_irq_frame:
    leaq 136(%rdi), %rsp
    iretq

.halt:
    hlt
    jmp .halt
