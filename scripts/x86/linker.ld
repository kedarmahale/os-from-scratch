/* scripts/x86/linker.ld - x86 kernel Linker script */

ENTRY(_start)

SECTIONS
{
    /* Start at 1MB (traditional kernel load address) */
    . = 1M;

    /* Mark the beginning of the kernel for memory management */
    _kernel_start = .;

    /* First put the multiboot header */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Mark the end of the kernel - THIS WAS MISSING! */
    _kernel_end = .;
    
    /* Also provide kernel size calculation */
    _kernel_size = _kernel_end - _kernel_start;
}
