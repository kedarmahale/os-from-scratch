# ============================================================================
# MEOWKERNEL x86 BOOT SEQUENCE - MULTIBOOT COMPLIANT
# =============================================================================

.set ALIGN,    1<<0                 # align loaded modules on page boundaries
.set MEMINFO,  1<<1                 # provide memory map
.set FLAGS,    ALIGN | MEMINFO      # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002           # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS)     # checksum of above, to prove we are multiboot

# =============================================================================
# MULTIBOOT HEADER
# =============================================================================

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# =============================================================================
# STACK SETUP
# =============================================================================

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB stack
stack_top:

# =============================================================================
# KERNEL ENTRY POINT - CRITICAL MULTIBOOT PRESERVATION
# =============================================================================

.section .text
.global _start
.type _start, @function

_start:
    # CRITICAL: Save multiboot information immediately
    # EAX contains the multiboot magic number
    # EBX contains the address of the multiboot information structure
    
    # Validate multiboot magic number first
    cmp $0x2BADB002, %eax
    jne .no_multiboot_panic
    
    # Save multiboot info pointer (EBX) - CRITICAL for memory detection
    mov %ebx, multiboot_info_ptr
    mov %eax, multiboot_magic_value
    
    # Set up stack (ESP must be 16-byte aligned for C code)
    mov $stack_top, %esp
    
    # Clear direction flag (required by System V ABI)
    cld
    
    # Push multiboot parameters for kernel_main
    push %ebx    # multiboot_info structure pointer
    push %eax    # multiboot magic number
    
    # Call the kernel main function
    call kernel_main
    
    # If kernel_main returns, halt the system
    cli
    hlt
    jmp .halt_loop

.no_multiboot_panic:
    # Handle case where we weren't loaded by multiboot-compliant bootloader
    # This creates a visible pattern on screen
    mov $0xb8000, %edi      # VGA text buffer
    mov $0x4F45, %ax        # 'E' on red background (Error)
    mov %ax, 0(%edi)
    mov $0x4F52, %ax        # 'R' on red background  
    mov %ax, 2(%edi)
    mov $0x4F52, %ax        # 'R' on red background
    mov %ax, 4(%edi)
    cli
    hlt

.halt_loop:
    hlt
    jmp .halt_loop

# =============================================================================
# MULTIBOOT INFORMATION STORAGE
# =============================================================================

.section .data
.global multiboot_info_ptr
.global multiboot_magic_value

multiboot_info_ptr:
    .long 0

multiboot_magic_value:
    .long 0

# =============================================================================
# GDT SETUP (Basic flat memory model)
# =============================================================================

.section .rodata

gdt_start:
    # Null descriptor (required)
    .quad 0

gdt_code:
    # Code segment: base=0, limit=0xfffff, present, ring 0, code, readable
    .word 0xffff    # limit low
    .word 0x0000    # base low  
    .byte 0x00      # base middle
    .byte 0x9a      # access (present, ring 0, code, readable)
    .byte 0xcf      # granularity (4KB blocks, 32-bit)
    .byte 0x00      # base high

gdt_data:
    # Data segment: base=0, limit=0xfffff, present, ring 0, data, writable  
    .word 0xffff    # limit low
    .word 0x0000    # base low
    .byte 0x00      # base middle  
    .byte 0x92      # access (present, ring 0, data, writable)
    .byte 0xcf      # granularity (4KB blocks, 32-bit)
    .byte 0x00      # base high

gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1    # GDT size
    .long gdt_start                   # GDT address

# Code and data segment selectors
.set CODE_SEG, gdt_code - gdt_start
.set DATA_SEG, gdt_data - gdt_start

