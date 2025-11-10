;
; Stage 1 Bootloader with VBE mode setting
;
[ORG 0x7c00]
[BITS 16]

CODE_SEGMENT equ 0x07e0

jmp short start
nop

; VBE Info Block structure
vbe_info:
    .width dw 0
    .height dw 0
    .bpp db 0
    ._padding db 0 ; Padding to align the struct
    .bytes_per_line dd 0
    .physical_buffer dd 0

start:
    ; Setup segments and stack
    cli
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    ; Store boot drive (BIOS passes it in dl)
    mov [boot_drive], dl

    ; Set a VBE graphics mode (e.g., 1024x768x32)
    mov ax, 0x4F02      ; VBE Set Mode function
    mov bx, 0x118       ; 1024x768x32bpp mode number (common, but not guaranteed)
    ; Bit 14: Use Linear Frame Buffer
    or bx, (1 << 14)
    int 0x10            ; Call video BIOS

    ; Check if the call was successful (al=0x4F, ah=0x00)
    cmp ax, 0x004F
    jne .mode_set_failed

    ; Get VBE Mode Information
    mov ax, 0x4F01      ; VBE Get Mode Info function
    mov cx, 0x118       ; Mode number
    mov di, vbe_mode_info_buffer ; Buffer to store info
    int 0x10

    ; Check for success again
    cmp ax, 0x004F
    jne .mode_set_failed

    ; Copy the relevant info to our kernel-compatible structure
    mov ax, [vbe_mode_info_buffer + 18] ; Width
    mov [vbe_info.width], ax
    mov ax, [vbe_mode_info_buffer + 20] ; Height
    mov [vbe_info.height], ax
    mov al, [vbe_mode_info_buffer + 25] ; BPP
    mov [vbe_info.bpp], al
    mov eax, [vbe_mode_info_buffer + 16] ; Bytes per line (Pitch)
    mov [vbe_info.bytes_per_line], eax
    mov eax, [vbe_mode_info_buffer + 40] ; Framebuffer physical address
    mov [vbe_info.physical_buffer], eax

    ; Load stage 2 from disk
    mov ah, 0x02        ; BIOS read sectors function
    mov al, 20          ; Sectors to read
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start at sector 2 (sector 1 is this bootloader)
    mov dh, 0           ; Head 0
    mov dl, [boot_drive]
    mov bx, CODE_SEGMENT
    mov es, bx
    mov bx, 0
    int 0x13
    jc .disk_error

    ; Jump to stage 2
    ; We need to pass boot_drive and the address of vbe_info
    push word [boot_drive]
    push vbe_info
    call CODE_SEGMENT:0

    ; Should not return
    jmp .halt

.disk_error:
    ; Print 'D' for disk error
    mov si, disk_error_msg
    jmp .print_and_halt

.mode_set_failed:
    ; Print 'V' for VBE error
    mov si, vbe_error_msg
    jmp .print_and_halt

.print_and_halt:
    mov ah, 0x0e
.print_loop:
    lodsb
    cmp al, 0
    je .halt
    int 0x10
    jmp .print_loop

.halt:
    cli
    hlt

boot_drive db 0
disk_error_msg db 'Disk Error', 0
vbe_error_msg db 'VBE Error', 0

vbe_mode_info_buffer: times 256 db 0

times 510-($-$$) db 0
dw 0xAA55