.section .text
.code64

.global x86_64_context_switch
.type x86_64_context_switch, @function
x86_64_context_switch:
    /* SysV ABI: rdi = current context, rsi = next context. */
    testq %rdi, %rdi
    jz .restore

    movq %rsp, 0(%rdi)
    leaq .resume(%rip), %rax
    movq %rax, 8(%rdi)
    pushfq
    popq %rax
    movq %rax, 16(%rdi)
    movq %rbx, 24(%rdi)
    movq %rbp, 32(%rdi)
    movq %r12, 40(%rdi)
    movq %r13, 48(%rdi)
    movq %r14, 56(%rdi)
    movq %r15, 64(%rdi)

.restore:
    /* A null next context is a programming error; do not return to it. */
    testq %rsi, %rsi
    jz .halt
    movq 24(%rsi), %rbx
    movq 32(%rsi), %rbp
    movq 40(%rsi), %r12
    movq 48(%rsi), %r13
    movq 56(%rsi), %r14
    movq 64(%rsi), %r15
    movq 0(%rsi), %rsp
    pushq 16(%rsi)
    popfq
    jmp *8(%rsi)

.resume:
    ret

.halt:
    cli
1:
    hlt
    jmp 1b

/* Initial contexts enter here with r12 = entry and r13 = argument. */
.global x86_64_context_start
.type x86_64_context_start, @function
x86_64_context_start:
    /* Align the stack for the call according to the SysV x86_64 ABI. */
    subq $8, %rsp
    movq %r13, %rdi
    call *%r12
    addq $8, %rsp
    cli
1:
    hlt
    jmp 1b

/* Scheduler-created tasks enable preemption only after entering on a valid
 * synthetic interrupt frame.  The cooperative context self-test uses the
 * non-preempting entry point above. */
.global x86_64_scheduler_start
.type x86_64_scheduler_start, @function
x86_64_scheduler_start:
    /* TEMP DEBUG: prove that the first iret reached the task entry point. */
    movw $0x3f8, %dx
    movb $'S', %al
    outb %al, (%dx)
    sti
    subq $8, %rsp
    movq %r13, %rdi
    call *%r12
    addq $8, %rsp
    cli
1:
    hlt
    jmp 1b
