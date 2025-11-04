bits 16

section .entry

extern __bss_start
extern __end

; C functions from our bootloader library
extern DISK_Initialize
extern FAT_Initialize
extern FAT_Open
extern FAT_Read
extern FAT_Close

global entry

entry:
    cli

    ; save boot drive
    mov [g_BootDrive], dl

    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; Graphic Mode STILL WORK IN PROGRESS Edit: Nov 4 2025 - Fuckkkkk
    ; call graphicsSwitch
    ;call graphicsSwitch
    
    ; Set a VESA graphics mode (e.g., 1024x768x32)
    mov ax, 1024
    mov bx, 768
    mov cl, 32
    call vbe_set_mode

    ; switch to protected mode
    call EnableA20          ; 2 - Enable A20 gate
    call LoadGDT            ; 3 - Load GDT

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

    ; Load the kernel from disk
    ; This logic is moved from the old bootloader's main.c
    mov esp, 0x7C00 ; Set up a temporary stack for C calls
    pusha
    mov bp, sp
    
    push dword [g_BootDrive]
    lea eax, [disk_struct]
    push eax
    call DISK_Initialize
    add esp, 8

    lea eax, [disk_struct]
    push eax
    call FAT_Initialize
    add esp, 4

    push dword kernel_filename
    lea eax, [disk_struct]
    push eax
    call FAT_Open
    add esp, 8
    mov [fd_struct], eax

    call load_kernel_loop

    popa

    ; Now, call the loaded kernel with the correct arguments
    push vbe_screen
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call 0x100000 ; Call kernel at its loaded address

    cli
    hlt


EnableA20:
    [bits 16]
    ; disable keyboard
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    ; read control output port
    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    ; write control output port
    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al
    
    call A20WaitInput
    pop eax
    or al, 2                                    ; bit 2 = A20 bit
    out KbdControllerDataPort, al

    ; enable keyboard
    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput
    ret


A20WaitInput:
    [bits 16]
    ; wait until status bit 2 (input buffer) is 0
    ; by reading from command port, we read status byte
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    [bits 16]
    ; wait until status bit 1 (output buffer) is 1 so it can be read
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret


LoadGDT:
    [bits 16]
    lgdt [g_GDTDesc]
    ret



KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1

ScreenBuffer                        equ 0xB8000

g_GDT:      ; NULL descriptor
            dq 0

            ; 32-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 32-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

g_GDTDesc:  dw g_GDTDesc - g_GDT - 1    ; limit = size of GDT
            dd g_GDT                    ; address of GDT

g_BootDrive: db 0

graphicsSwitch: 
    ; switch to graphics mode 0x13
    mov ax, 0x0013
    int 0x10
    ret

; Sets a VESA mode
; IN: AX = width, BX = height, CL = bpp
; OUT: Carry flag set on error
vbe_set_mode:
    mov [.width], ax
    mov [.height], bx
    mov [.bpp], cl

    call find_mode
    jc .error

    call set_mode
    jc .error

    clc
    ret

.error:
    stc
    ret

.width  dw 0
.height dw 0
.bpp    db 0
.mode   dw 0

find_mode:
    push es
    mov ax, 0x4F00
    mov di, vbe_info_block
    int 0x10
    pop es
    cmp ax, 0x4F
    jne .error

    mov ax, [vbe_info_block.video_modes_ptr + 2]
    mov es, ax
    mov si, [vbe_info_block.video_modes_ptr]

.find_loop:
    mov ax, [es:si]
    add si, 2
    cmp ax, 0xFFFF
    je .error

    push es
    mov cx, ax
    mov ax, 0x4F01
    mov di, mode_info_block
    int 0x10
    pop es
    cmp ax, 0x4F
    jne .error

    mov ax, [vbe_set_mode.width]
    cmp ax, [mode_info_block.width]
    jne .find_loop

    mov ax, [vbe_set_mode.height]
    cmp ax, [mode_info_block.height]
    jne .find_loop

    mov al, [vbe_set_mode.bpp]
    cmp al, [mode_info_block.bpp]
    jne .find_loop

    mov ax, [es:si-2]
    mov [vbe_set_mode.mode], ax
    clc
    ret

.error:
    stc
    ret

set_mode:
    ; Populate the vbe_screen structure in the exact order the C kernel expects.
    mov ax, [vbe_set_mode.width]
    mov [vbe_screen.width], ax
    mov ax, [vbe_set_mode.height]
    mov [vbe_screen.height], ax
    mov al, [vbe_set_mode.bpp]
    mov [vbe_screen.bpp], al
    movzx eax, al
    shr eax, 3
    mov [vbe_screen.bytes_per_pixel], eax
    mov ax, [mode_info_block.pitch]
    mov [vbe_screen.bytes_per_line], ax
    mov eax, [mode_info_block.framebuffer]
    mov [vbe_screen.physical_buffer], eax

    push es
    mov ax, 0x4F02
    mov bx, [vbe_set_mode.mode]
    or bx, 0xC000 ; enable Linear Frame Buffer and don't clear video memory
    mov di, 0
    int 0x10
    pop es
    cmp ax, 0x4F
    jne .error

    clc
    ret

.error:
    stc
    ret

load_kernel_loop:
    pusha
.loop:
    push dword 0x10000 ; Buffer to read into (MEMORY_LOAD_KERNEL)
    push dword 4096    ; Bytes to read (MEMORY_LOAD_SIZE)
    push dword [fd_struct]
    lea eax, [disk_struct]
    push eax
    call FAT_Read
    add esp, 16
    test eax, eax
    jz .done
    ; memcpy is complex, for now we assume kernel is small and fits in one read
    ; A proper implementation would copy from the load buffer to the final kernel address.
.done:
    popa
    ret

section .data
; This structure holds the VBE mode information passed to the kernel.
; Moved from .bss to .data to prevent the kernel's BSS clear from zeroing it.
vbe_screen:
    .width           resw 1
    .height          resw 1
    .bpp             resb 1
    .bytes_per_pixel resd 1
    .bytes_per_line  resw 1
    .physical_buffer resd 1

kernel_filename: db "/kernel.bin", 0

section .bss

vbe_info_block:
    .signature       resb 4
    .version         resw 1
    .oem_string_ptr  resd 1
    .capabilities    resd 1
    .video_modes_ptr resd 1
    .total_memory    resw 1
    .oem_sw_rev      resw 1
    .oem_vendor_ptr  resd 1
    .oem_product_ptr resd 1
    .oem_rev_ptr     resd 1
                     resb 222
                     resb 256

mode_info_block:
    .attributes      resw 1
    .window_a        resb 1
    .window_b        resb 1
    .granularity     resw 1
    .window_size     resw 1
    .segment_a       resw 1
    .segment_b       resw 1
    .win_func_ptr    resd 1
    .pitch           resw 1
    .width           resw 1
    .height          resw 1
    .char_width      resb 1
    .char_height     resb 1
    .planes          resb 1
    .bpp             resb 1
    .banks           resb 1
    .memory_model    resb 1
    .bank_size       resb 1
    .image_pages     resb 1
    .reserved1       resb 1
    .red_mask        resb 1
    .red_position    resb 1
    .green_mask      resb 1
    .green_position  resb 1
    .blue_mask       resb 1
    .blue_position   resb 1
    .reserved_mask   resb 1
    .reserved_pos    resb 1
    .dcf             resb 1
    .framebuffer     resd 1
                     resb 206

disk_struct: resb 128 ; Reserve space for DISK struct
fd_struct:   resd 1   ; Reserve space for FAT_File*
