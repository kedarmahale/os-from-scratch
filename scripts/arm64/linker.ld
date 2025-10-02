/* scripts/arm64/linker.ld - ARM64 linker script for Raspberry Pi */

ENTRY(_start)

SECTIONS
{
    /* Raspberry Pi 3/4/5 kernel load address */
    . = 0x80000;
    
    _kernel_start = .;
    
    /* Boot code must be first */
    .text.boot : {
        KEEP(*(.text.boot))
    }
    
    /* Regular code */
    .text : {
        *(.text)
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata)
    }
    
    /* Initialized data */
    .data : {
        *(.data)
    }
    
    /* Uninitialized data */
    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    }
    
    _kernel_end = .;
    _kernel_size = _kernel_end - _kernel_start;
}
