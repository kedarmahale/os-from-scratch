; bootloader.asm
BITS 16
ORG 0x7C00                                      ; BIOS loads here

start:
    mov si, hello_msg
    call print_string

hang:
    cli                                         ; clear interrupts
    hlt                                         ; halt CPU
    jmp hang

print_string:
    mov ah, 0x0E                                ; BIOS teletype function (int 10h)
.next_char:
    lodsb                                       ; load byte from DS:SI into AL
    cmp al, 0
    je .done
    int 0x10                                    ; print character in AL
    jmp .next_char                      
.done:
    ret

hello_msg db "Hello from bootloader", 0
        
times 510 - ($ - $$) db 0                       ; pad to 510 bytes
dw 0xAA55                                       ; boot signature
