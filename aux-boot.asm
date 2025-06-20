; same bootloader implementation now with VGA text mode access

[BITS 16]
[ORG 0x7C00]

start:
; Clear interrups and set up segments cli
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x7C00   

; Clear the screen and display welcome message
    call clear_screen
    mov si, welcome_msg
    mov bl, 0x0F            ; White text on black background
    call print_string_color

    ;status message
    mov si, status_msg
    mov bl, 0x0A            ; Green text
    call print_string_color

    call load_gdt
    call switch_to_protected_mode

    jmp$

; Function: Clear screen using VGA memory
clear_screen:
    pusha
    mov ax, 0xB800          
    mov es, ax
    mov cx, 2000         
    mov ax, 0x0720

clear_loop:
    stosw                  
    loop clear_loop
    
    popa
    ret

; Function: Print colored string directly to VGA memory
; SI = string pointer, BL = color attribute
print_string_color:
    pusha
    mov ax, 0xB800
    mov es, ax
    mov di, 0               
    
print_loop:
    lodsb        
    cmp al, 0               
    je print_done
    
    ; Store character and color
    stosb                
    mov al, bl            
    stosb                 
    
    jmp print_loop
    
print_done:
    popa
    ret

; Function: Load Global Descriptor Table
load_gdt:
    lgdt [gdt_descriptor]
    ret

; Function: Switch to Protected Mode
switch_to_protected_mode:
    ; Disable interrupts
    cli
    
    ; Enable A20 line (allows access to memory above 1MB)
    call enable_a20
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Far jump to flush prefetch queue and enter protected mode
    jmp CODE_SEG:protected_mode_start

; Function: Enable A20 line
enable_a20:
    ; Method 1: Keyboard controller
    call wait_8042
    mov al, 0xAD
    out 0x64, al            ; Disable keyboard
    
    call wait_8042
    mov al, 0xD0
    out 0x64, al            ; Read output port
    
    call wait_8042_data
    in al, 0x60
    push eax
    
    call wait_8042
    mov al, 0xD1
    out 0x64, al            ; Write output port
    
    call wait_8042
    pop eax
    or al, 2                ; Set A20 bit
    out 0x60, al
    
    call wait_8042
    mov al, 0xAE
    out 0x64, al            ; Enable keyboard
    
    call wait_8042
    ret

wait_8042:
    in al, 0x64
    test al, 2
    jnz wait_8042
    ret

wait_8042_data:
    in al, 0x64
    test al, 1
    jz wait_8042_data
    ret

; Data section
welcome_msg: db "Welcome to Shriya's Bootloader!", 13, 10, 0
status_msg:  db "Switching to Protected Mode...", 13, 10, 0

; Global Descriptor Table
gdt_start:
    ; Null descriptor (required)
    dd 0x0
    dd 0x0

gdt_code:
    ; Code segment descriptor
    ; Base: 0x0, Limit: 0xFFFFF
    ; Access: Present, Ring 0, Code, Execute/Read
    ; Flags: 32-bit, 4KB granularity
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)

gdt_data:
    ; Data segment descriptor
    ; Same as code but with different access rights
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10010010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1    ; GDT size
    dd gdt_start                  ; GDT address

; Constants for segment selectors
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; 32-bit Protected Mode code starts here
[BITS 32]
protected_mode_start:
    ; Set up data segments
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Clear screen in protected mode
    call clear_screen_32
    
    ; Display success message
    call print_success_message
    
    ; Infinite loop
    jmp $

; 32-bit function to clear screen
clear_screen_32:
    mov edi, 0xB8000
    mov ecx, 2000
    mov eax, 0x0F200F20     ; Two spaces with white on black
    
clear_loop_32:
    stosd
    loop clear_loop_32
    ret

; 32-bit function to print success message
print_success_message:
    mov esi, success_msg_32
    mov edi, 0xB8000
    mov ah, 0x0A            ; Green text
    
print_loop_32:
    lodsb
    cmp al, 0
    je print_done_32
    stosb                   ; Store character
    mov al, ah              ; Load color
    stosb                   ; Store color
    mov al, [esi-1]         ; Restore character for next iteration
    jmp print_loop_32
    
print_done_32:
    ret

success_msg_32: db 'SUCCESS: Now running in 32-bit Protected Mode!', 0

; Fill remaining space and add boot signature
times 510-($-$$) db 0
dw 0xAA55

