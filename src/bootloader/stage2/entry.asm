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
    mov sp, 0xFFFFF ;0xFFF0
    mov bp, sp

    ; set a VBE graphics mode (e.g., 1024x768x32bpp)
    mov ax, 1920    ; width
    mov bx, 1080    ; height
    mov cl, 32      ; bpp
    call vbe_set_mode
    ; jc .vbe_error ; uncomment to handle VBE errors

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

    ; --- Draw a test pixel from assembly ---
    ; Draw a RED pixel at (x=50, y=50) to verify VBE mode
    mov edi, [vbe_screen.physical_buffer]   ; Get framebuffer base address
    movzx eax, word [vbe_screen.pitch]      ; eax = bytes per scanline (pitch)
    mov ebx, 50                             ; y = 50
    mul ebx                                 ; eax = y * pitch
    mov ebx, 50                             ; x = 50
    shl ebx, 2                              ; ebx = x * 4 (for 32 bpp)
    add eax, ebx                            ; eax = y * pitch + x * 4
    add edi, eax                            ; edi = framebuffer + offset
    mov dword [edi], 0x00FF0000             ; Draw a red pixel (0x00RRGGBB)
    ; --- End of test pixel code ---
   
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

; --- VBE ---

; vbe_set_mode:
; Sets a VESA mode
; In:   AX = Width
; In:   BX = Height
; In:   CL = Bits per pixel
; Out:  FLAGS = Carry clear on success, set on failure
; Out:  vbe_screen structure is filled on success
vbe_set_mode:
    [bits 16]
    mov [.width], ax
    mov [.height], bx
    mov [.bpp], cl

    sti ; some BIOSes need interrupts for VBE calls

    push es ; some VESA BIOSes destroy ES
    mov ax, ds
    mov es, ax ; ES must point to our data segment for the BIOS call
    mov ax, 0x4F00 ; get VBE BIOS info
    mov di, vbe_info_block ; DI is the offset in the ES segment
    int 0x10
    pop es
    cli

    cmp ax, 0x4F ; BIOS doesn't support VBE?
    jne .error

    ; Get pointer to video modes list
    mov ax, [vbe_info_block.video_modes + 2] ; segment
    mov es, ax
    mov si, [vbe_info_block.video_modes]     ; offset

.find_mode:
    mov dx, [es:si]
    add si, 2

    cmp dx, 0xFFFF ; end of list?
    je .error

    push es
    mov ax, ds
    mov es, ax ; ES must point to our data segment for the BIOS call
    mov ax, 0x4F01 ; get VBE mode info
    mov cx, dx     ; mode number to query
    mov di, mode_info_block
    int 0x10
    pop es

    cmp ax, 0x4F
    jne .next_mode ; if call fails, try next mode

    ; Check if mode attributes match what we want
    mov ax, [.width]
    cmp ax, [mode_info_block.width]
    jne .next_mode

    mov ax, [.height]
    cmp ax, [mode_info_block.height]
    jne .next_mode

    mov al, [.bpp]
    cmp al, [mode_info_block.bpp]
    jne .next_mode

    ; Found a suitable mode!
    ; Populate our vbe_screen structure
    mov ax, [mode_info_block.width]
    mov [vbe_screen.width], ax
    mov ax, [mode_info_block.height]
    mov [vbe_screen.height], ax
    mov eax, [mode_info_block.framebuffer]
    mov [vbe_screen.physical_buffer], eax
    mov ax, [mode_info_block.pitch]
    mov [vbe_screen.pitch], ax
    mov al, [mode_info_block.bpp]
    mov [vbe_screen.bpp], al

    ; Set the mode
    mov ax, 0x4F02
    mov bx, dx
    or bx, 0x4000 ; enable Linear Frame Buffer (LFB)
    int 0x10

    cmp ax, 0x4F
    jne .error

    clc ; success
    ret

.next_mode:
    jmp .find_mode

.error:
    stc ; failure
    ret

; Local variables for vbe_set_mode
.width  dw 0
.height dw 0
.bpp    db 0

section .bss

; VBE Info Block (VBE 2.0+)
vbe_info_block:
    .signature       resb 4
    .version         resw 1
    .oem_string_ptr  resd 1
    .capabilities    resd 1
    .video_modes     resd 1
    .total_memory    resw 1
    .oem_sw_rev      resw 1
    .oem_vendor_name resd 1
    .oem_prod_name   resd 1
    .oem_prod_rev    resd 1
    .reserved        resb 222
    .oem_data        resb 256

; VBE Mode Info Block
mode_info_block:
    .attributes      resw 1
    .window_a        resb 2
    .granularity     resw 1
    .window_size     resw 1
    .segment_a       resw 1
    .segment_b       resw 1
    .win_func_ptr    resd 1
    .pitch           resw 1
    .width           resw 1
    .height          resw 1
    .w_char          resb 1
    .y_char          resb 1
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
    .rsv_mask        resb 1
    .rsv_position    resb 1
    .direct_color    resb 1
    .framebuffer     resd 1
    .off_screen_mem  resd 1
    .off_screen_size resw 1
    .reserved2       resb 206

section .data

; This structure will hold the graphics mode info for the kernel
vbe_screen:
    .width            dw 0
    .height           dw 0
    .pitch            dw 0
    .bpp              db 0
    .physical_buffer  dd 0