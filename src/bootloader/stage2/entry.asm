bits 16

section .entry
 
; C entry point
extern start

global entry
global vbe_screen

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
    
    ; Set a VESA graphics mode (e.g., 1024x768x32) It works the ax bx cl
    mov ax, 1024
    mov bx, 768
    mov cl, 32
    call vbe_set_mode
    jc .vbe_failed ; If VBE setup fails, print an error and halt.

    ; switch to protected mode
    call EnableA20          ; 2 - Enable A20 gate
    call LoadGDT            ; 3 - Load GDT
    call LoadIDT            ; Load a basic IDT to prevent triple faults

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers ; ONE PIXEl IS THE BEST THING IN MY LIFE RN I HAVE ONE!!
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- VBE DEBUG: Draw one white pixel at (0,16) ---
    ; If these pixels appear, VBE setup is correct.
    ; We draw Red, Green, Blue at (0,0), (1,0), (2,0)
    mov edi, [vbe_screen.physical_buffer] ; Get the framebuffer address
    mov dword [edi],     0x00FF0000      ; Red pixel at (0,0)
    mov dword [edi + 4], 0x0000FF00      ; Green pixel at (1,0)
    mov dword [edi + 8], 0x000000FF      ; Blue pixel at (2,0)

    ; Set up stack and call our C entry point
    mov esp, 0x70000 ; Stack grows downwards, so start it at the top of a safe region.
    xor ebp, ebp     ; C functions will set up their own base pointer. Zeroing it is good practice.
    push word [g_BootDrive] ; Pass boot drive as argument to start()
    call start
    ; The start() function should not return. If it does, something is wrong.
    ; We will fall through to the kernel return failure code.

    ; --- PRE-KERNEL CALL CANARY ---
    ; If this white pixel appears, it means kernel loading is complete and
    ; we are about to jump to the kernel's entry point. We draw white at (3,0).
    mov edi, [vbe_screen.physical_buffer] ; Get the framebuffer address
    mov dword [edi + 12], 0x00FFFFFF      ; Write a white pixel

    ; The 'start' function in main.c now handles jumping to the kernel.

    ; --- KERNEL RETURN/FAIL DEBUG ---
    ; If this red pixel appears, it means the kernel
    ; call returned. We draw a large yellow square to indicate this.
    mov edi, [vbe_screen.physical_buffer] ; EDI = Base framebuffer address
    movzx ebx, word [vbe_screen.bytes_per_line] ; EBX = pitch (bytes per line)

    mov ecx, 32 ; Height of the square (32 rows)
.fail_y_loop:
    push edi ; Save current line's starting address
    push ecx ; Save outer loop counter

    mov ecx, 32 ; Width of the square (32 pixels)
.fail_x_loop:
    mov dword [edi], 0x00FFFF00 ; Draw yellow pixel (Red+Green)
    add edi, 4 ; Move to next pixel in the row
    loop .fail_x_loop

    pop ecx ; Restore outer loop counter
    pop edi ; Restore line's starting address
    add edi, ebx ; Move to the start of the next line
    loop .fail_y_loop
    ; --- END KERNEL RETURN/FAIL DEBUG ---

    cli
    hlt

.vbe_failed:
    ; If VBE fails, we can't draw pixels. Fall back to text mode to show an error.
    mov ax, 0x0003  ; Standard 80x25 text mode
    int 0x10

    mov si, vbe_error_msg
    mov ah, 0x0E    ; Teletype output function
    mov bh, 0       ; Page number
.char_loop:
    lodsb           ; Load character from SI into AL
    test al, al     ; Check for null terminator
    jz .halt
    int 0x10        ; Print character
    jmp .char_loop
.halt:
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

LoadIDT:
    [bits 16]
    lidt [g_IDTDesc]
    ret

KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1

MEMORY_KERNEL_ADDR                  equ 0x100000
MEMORY_LOAD_KERNEL                  equ 0x30000
MEMORY_LOAD_SIZE                    equ 4096

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

g_IDT:      ; A basic, empty IDT. The kernel will fill this in.
g_IDTDesc:  dw (g_IDTDesc - g_IDT - 1)  ; Limit (size of IDT)
            dd g_IDT                    ; Base address of IDT


g_BootDrive: db 0

g_GDT_end:
g_GDTDesc:  dw g_GDT_end - g_GDT - 1    ; limit = size of GDT
            dd g_GDT                    ; address of GDT


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

    ; Set ES to CS so that the BIOS writes to our data segment.
    mov ax, cs
    mov es, ax

    mov ax, 0x4F00
    mov di, vbe_info_block
    int 0x10
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

    mov cx, ax
    mov ax, 0x4F01
    ; IMPORTANT: Reset ES to our data segment before this BIOS call.
    ; The BIOS will write to the buffer at ES:DI.
    push cs
    pop es
    mov di, mode_info_block
    int 0x10
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
    movzx eax, al ; bpp (e.g., 32)
    shr eax, 3
    mov [vbe_screen.bytes_per_pixel], eax
    mov ax, [mode_info_block.pitch]
    mov [vbe_screen.bytes_per_line], ax
    ; This is the key instruction:
    ; It reads the framebuffer's physical address from the mode_info_block...
    mov eax, [mode_info_block.framebuffer]
    mov [vbe_screen.physical_buffer], eax ; ...and stores it in our vbe_screen struct.

    mov ax, 0x4F02
    mov bx, [vbe_set_mode.mode]
    or bx, 0x4000 ; enable Linear Frame Buffer and clear video memory
    xor edi, edi ; Per VBE spec, ES:DI should be 0 if not passing CRTC info.
                 ; Zeroing the full 32-bit register is good practice.
    int 0x10
    cmp ax, 0x4F
    jne .error

    clc
    ret

.error:
    stc
    ret

section .data

vbe_error_msg: db "VBE Error: Failed to set graphics mode.", 0

section .bss
; This structure holds the VBE mode information passed to the kernel.
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

vbe_screen:
    .width           resw 1
    .height          resw 1
    .bpp             resb 1
                     resb 3  ; Padding to align bytes_per_pixel to a 4-byte boundary
    .bytes_per_pixel resd 1
    .bytes_per_line  resw 1
                     resb 2  ; Padding to align physical_buffer
    .physical_buffer resd 1
