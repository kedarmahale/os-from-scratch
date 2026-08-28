.set MULTIBOOT_MAGIC, 0x1BADB002
.set MULTIBOOT_FLAGS, 0x3
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
.section .multiboot
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
.section .bss
.align 16
stack_bottom: .skip 16384
stack_top:
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd: .skip 4096
.section .text
.code32
.global _start
_start:
    cli
    cld
    mov %eax, %esi
    mov %ebx, multiboot_info_value

    # Multiboot enters us in 32-bit protected mode. Verify that this CPU can
    # execute the long-mode transition before changing any control registers.
    mov $0x80000000, %eax
    cpuid
    cmp $0x80000001, %eax
    jb .no_long_mode
    mov $0x80000001, %eax
    cpuid
    test $0x20000000, %edx
    jz .no_long_mode

    mov $stack_top, %esp

    # Build an identity map for the first 1 GiB using 2 MiB pages:
    # PML4[0] -> PDPT[0] -> PD[0..511].
    mov $pml4, %eax
    mov $pdpt, %ebx
    or $0x3, %ebx
    mov %ebx, (%eax)
    movl $0, 4(%eax)

    mov $pdpt, %eax
    mov $pd, %ebx
    or $0x3, %ebx
    mov %ebx, (%eax)
    movl $0, 4(%eax)

    mov $pdpt, %eax
    mov $pd, %edi
    xor %ecx, %ecx
.fill_pd:
    mov %ecx, %eax
    shl $21, %eax
    or $0x83, %eax
    mov %eax, (%edi,%ecx,8)
    movl $0, 4(%edi,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne .fill_pd

    mov $pml4, %eax
    mov %eax, %cr3

    # Enable PAE, SSE save/restore, and SIMD exception support.
    mov %cr4, %eax
    or $0x620, %eax
    mov %eax, %cr4

    # Enable long mode in EFER.
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    # Enable paging and protected-mode floating-point support. EM and TS must
    # be clear before the first SSE/x87 instruction in C code.
    mov %cr0, %eax
    and $~0xC, %eax
    or $0x80000023, %eax
    mov %eax, %cr0
    lgdt gdt64_descriptor
    push $0x08
    push $long_mode_entry
    lret
.code64
long_mode_entry:
    mov $0x10, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %ss
    fninit
    mov %esi, %ebx                 # preserve magic across the C IDT init call
    call x86_64_idt_init
    mov %ebx, %edi
    mov multiboot_info_value, %esi
    call kernel_main64
.halt:
    hlt
    jmp .halt
.section .rodata
.align 8
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF
gdt64_end:
gdt64_descriptor:
    .word gdt64_end - gdt64 - 1
    .long gdt64
.section .data
multiboot_info_value:
    .long 0

.section .rodata
no_long_mode_message:
    .asciz "MeowKernel: x86_64 long mode is not supported"

.section .text
.code32
.no_long_mode:
    mov $0xb8000, %edi
    mov $no_long_mode_message, %esi
.no_long_mode_write:
    mov (%esi), %al
    test %al, %al
    jz .no_long_mode_halt
    mov $0x4f, %ah
    mov %ax, (%edi)
    add $2, %edi
    inc %esi
    jmp .no_long_mode_write
.no_long_mode_halt:
    cli
    hlt
    jmp .no_long_mode_halt
