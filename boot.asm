; boot.asm - Simple bootloader that loads and runs our kernel
; This bootloader loads the kernel from disk and jumps to it

[BITS 16]           ; We start in 16-bit real mode
[ORG 0x7C00]        ; BIOS loads us at 0x7C00

; Main bootloader entry point
start:
    ; Initialize segments
    cli             ; Clear interrupts
    xor ax, ax      ; Set AX to 0
    mov ds, ax      ; Set Data Segment to 0
    mov es, ax      ; Set Extra Segment to 0
    mov ss, ax      ; Set Stack Segment to 0
    mov sp, 0x7C00  ; Set stack pointer below bootloader
    sti             ; Enable interrupts
    
    ; Print loading message
    mov si, loading_msg
    call print_string
    
    ; Load kernel from disk
    ; We'll load it to 0x10000 (64KB) to avoid conflicts
    mov ax, 0x1000  ; Load segment (0x1000 * 16 = 0x10000)
    mov es, ax      ; Set ES to load segment
    mov bx, 0x0000  ; Offset within segment
    
    mov ah, 0x02    ; BIOS read sectors function
    mov al, 18      ; Number of sectors to read (9KB kernel)
    mov ch, 0       ; Cylinder 0
    mov cl, 2       ; Start from sector 2 (sector 1 is bootloader)
    mov dh, 0       ; Head 0
    mov dl, 0       ; Drive 0 (floppy A:)
    
    int 0x13        ; Call BIOS disk services
    jc disk_error   ; Jump if carry flag set (error)
    
    ; Print success message
    mov si, success_msg
    call print_string
    
    ; Enable A20 line (allows access to memory above 1MB)
    call enable_a20
    
    ; Switch to protected mode
    cli             ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load Global Descriptor Table
    
    mov eax, cr0    ; Get current CR0
    or eax, 1       ; Set PE bit (Protection Enable)
    mov cr0, eax    ; Enable protected mode
    
    ; Far jump to flush CPU pipeline and switch to 32-bit code
    jmp 0x08:protected_mode
    
disk_error:
    mov si, error_msg
    call print_string
    hlt             ; Halt the system

; Print string function (16-bit real mode)
print_string:
    lodsb           ; Load byte from SI into AL
    or al, al       ; Check if AL is 0
    jz .done        ; If zero, we're done
    mov ah, 0x0E    ; BIOS teletype function
    int 0x10        ; Call BIOS video services
    jmp print_string
.done:
    ret

; Enable A20 line
enable_a20:
    ; Try keyboard controller method
    call .wait_8042
    mov al, 0xAD    ; Disable keyboard
    out 0x64, al
    
    call .wait_8042
    mov al, 0xD0    ; Read output port
    out 0x64, al
    
    call .wait_8042_data
    in al, 0x60     ; Read data
    push ax         ; Save data
    
    call .wait_8042
    mov al, 0xD1    ; Write output port
    out 0x64, al
    
    call .wait_8042
    pop ax          ; Restore data
    or al, 2        ; Set A20 bit
    out 0x60, al    ; Write data
    
    call .wait_8042
    mov al, 0xAE    ; Enable keyboard
    out 0x64, al
    
    call .wait_8042
    ret

.wait_8042:
    in al, 0x64     ; Read status
    test al, 2      ; Check if input buffer full
    jnz .wait_8042  ; Wait if full
    ret

.wait_8042_data:
    in al, 0x64     ; Read status
    test al, 1      ; Check if output buffer empty
    jz .wait_8042_data ; Wait if empty
    ret

; 32-bit protected mode code
[BITS 32]
protected_mode:
    ; Set up data segments
    mov ax, 0x10    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, 0x90000    ; Set stack pointer to 576KB
    
    ; Copy kernel to 1MB (0x100000) where it expects to be
    mov esi, 0x10000    ; Source: where we loaded the kernel
    mov edi, 0x100000   ; Destination: 1MB mark
    mov ecx, 0x2400     ; Copy 9KB (18 sectors * 512 bytes)
    rep movsb           ; Copy byte by byte
    
    ; Jump to kernel
    jmp 0x100000        ; Jump to kernel entry point

; Global Descriptor Table
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0
    
    ; Code segment descriptor
    dw 0xFFFF       ; Limit (low)
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (middle)
    db 10011010b    ; Access (present, ring 0, code, execute/read)
    db 11001111b    ; Flags (4KB blocks, 32-bit)
    db 0x00         ; Base (high)
    
    ; Data segment descriptor
    dw 0xFFFF       ; Limit (low)
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (middle)
    db 10010010b    ; Access (present, ring 0, data, read/write)
    db 11001111b    ; Flags (4KB blocks, 32-bit)
    db 0x00         ; Base (high)
gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                ; GDT address

; Messages
loading_msg db 'Loading kernel...', 13, 10, 0
success_msg db 'Kernel loaded! Switching to protected mode...', 13, 10, 0
error_msg db 'Disk read error!', 13, 10, 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55           ; Boot signature