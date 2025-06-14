; System bootloader that displays a message
; This runs in a 16-bit real mode

[BITS 16]   ; NASM message that we operate on 16-bit bootloader
[ORG 0x7C00] ; Bootloaders are loaded at address Ox7C00
start:
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFFFF

    mov si, hello_msg
    call print_string

    jmp $

print_string:
    mov ah, 0x0E

.next_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .next_char

.done: 
    ret

hello_msg:
    db 'Hello, World! Successful bootloader!', 0

times 510-($-$$) db 0
; Padding for the rest of the 512-byte sector with zeros
dw 0xAA55
; Boot signature - BIOS looks for this to identify bootable sectors