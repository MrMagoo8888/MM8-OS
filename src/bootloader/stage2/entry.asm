bits 16

section .entry

extern __bss_start
extern __end

extern start
global vbe_screen
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

    ; set VBE mode 1920x1080x32
    mov ax, 1920
    mov bx, 1080
    mov cl, 32
    call vbe_set_mode
    ; carry flag will be set on error

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
   
    ; clear bss (uninitialized data)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; expect boot drive in dl, send it as argument to cstart function
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start

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

; vbe_set_mode: Sets a VESA mode
; In:   AX = Width, BX = Height, CL = Bits per pixel
; Out:  FLAGS = Carry clear on success, set on error.
;       vbe_screen structure is filled on success.
vbe_set_mode:
    [bits 16]
    mov [.width], ax
    mov [.height], bx
    mov [.bpp], cl

    sti

    push es
    mov ax, 0x4F00              ; get VBE BIOS info
    mov di, vbe_info_block
    int 0x10
    pop es

    cmp ax, 0x4F                ; BIOS doesn't support VBE?
    jne .error

    mov ax, word [vbe_info_block.video_modes_ptr]
    mov [.offset], ax
    mov ax, word [vbe_info_block.video_modes_ptr+2]
    mov [.segment], ax

    mov ax, [.segment]
    mov fs, ax
    mov si, [.offset]

.find_mode:
    mov dx, [fs:si]
    add si, 2
    mov [.offset], si
    mov [.mode], dx

    cmp word [.mode], 0xFFFF    ; end of list?
    je .error

    push es
    mov ax, 0x4F01              ; get VBE mode info
    mov cx, [.mode]
    mov di, mode_info_block
    int 0x10
    pop es

    cmp ax, 0x4F
    jne .next_mode ; Some BIOSes return failure for unsupported modes, so just try next

    ; Check if mode attributes match. We need linear framebuffer support.
    mov ax, [mode_info_block.attributes]
    test ax, (1 << 7) ; Is LFB available?
    jz .next_mode

    mov ax, [.width]
    cmp ax, [mode_info_block.width]
    jne .next_mode

    mov ax, [.height]
    cmp ax, [mode_info_block.height]
    jne .next_mode

    mov al, [.bpp]
    cmp al, [mode_info_block.bpp]
    jne .next_mode

    ; Found the mode, set it
    mov ax, 0x4F02
    mov bx, [.mode]
    or bx, 0x4000               ; enable Linear Frame Buffer
    int 0x10

    cmp ax, 0x4F
    jne .error

    ; Success, fill vbe_screen structure
    mov eax, [mode_info_block.framebuffer]
    mov [vbe_screen.physical_buffer], eax

    clc
    ret

.next_mode:
    mov ax, [.segment]
    mov fs, ax
    mov si, [.offset]
    jmp .find_mode

.error:
    stc
    ret

.width      dw 0
.height     dw 0
.bpp        db 0
.segment    dw 0
.offset     dw 0
.mode       dw 0


KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1

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

section .bss

; VBE Info Block
vbe_info_block:
    .signature          resb 4
    .version            resw 1
    .oem_string_ptr     resd 1
    .capabilities       resd 1
    .video_modes_ptr    resd 1
    .total_memory       resw 1
    .oem_software_rev   resw 1
    .oem_vendor_name_ptr resd 1
    .oem_product_name_ptr resd 1
    .oem_product_rev_ptr resd 1
    .reserved           resb 222
    .oem_data           resb 256

; VBE Mode Info Block
mode_info_block:
    .attributes         resw 1
    .window_a           resb 1
    .window_b           resb 1
    .granularity        resw 1
    .window_size        resw 1
    .segment_a          resw 1
    .segment_b          resw 1
    .win_func_ptr       resd 1
    .pitch              resw 1
    .width              resw 1
    .height             resw 1
    .char_width         resb 1
    .char_height        resb 1
    .planes             resb 1
    .bpp                resb 1
    .banks              resb 1
    .memory_model       resb 1
    .bank_size          resb 1
    .image_pages        resb 1
    .reserved0          resb 1
    .red_mask           resb 1
    .red_position       resb 1
    .green_mask         resb 1
    .green_position     resb 1
    .blue_mask          resb 1
    .blue_position      resb 1
    .reserved_mask      resb 1
    .reserved_position  resb 1
    .direct_color_attributes resb 1
    .framebuffer        resd 1
    .off_screen_mem_offset resd 1
    .off_screen_mem_size resw 1
    .reserved1          resb 206

; This structure will hold the graphics mode info for the kernel
vbe_screen:
    .physical_buffer    resd 1