bits 16

section .entry

extern __bss_start
extern __end

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

    mov ax, 1024
    mov bx, 768
    mov cl, 32
    call vbe_set_mode
    jc .vbe_failed

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
   
    ; Push arguments for the kernel's start function in reverse order.
    ; The kernel expects: start(bootDrive, vbe_screen_ptr)
    push vbe_screen
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start

    cli
    hlt

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

; vbe_set_mode:
; Sets a VESA mode
; In\	AX = Width
; In\	BX = Height
; In\	CL = Bits per pixel
; Out\	FLAGS = Carry clear on success
; Out\	Width, height, bpp, physical buffer, all set in vbe_screen structure

vbe_set_mode:
	mov [.width], ax
	mov [.height], bx
	mov [.bpp], cl

	sti

	push es					; some VESA BIOSes destroy ES, or so I read
	mov ax, 0x4F00				; get VBE BIOS info
	mov di, vbe_info_block
	int 0x10
	pop es

	cmp ax, 0x4F				; BIOS doesn't support VBE?
	jne .error

	mov ax, word[vbe_info_block.video_modes_ptr]
	mov [.offset], ax
	mov ax, word[vbe_info_block.video_modes_ptr+2]
	mov [.segment], ax

	mov ax, [.segment]
	mov fs, ax
	mov si, [.offset]

.find_mode:
	mov dx, [fs:si]
	add si, 2
	mov [.offset], si
	mov [.mode], dx
	mov ax, 0
	mov fs, ax

	cmp word [.mode], 0x0FFFF			; end of list?
	je .error

	push es
	mov ax, 0x4F01				; get VBE mode info
	mov cx, [.mode]
	mov di, mode_info_block
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

	mov ax, [.width]
	cmp ax, [mode_info_block.width]
	jne .next_mode

	mov ax, [.height]
	cmp ax, [mode_info_block.height]
	jne .next_mode

	mov al, [.bpp]
	cmp al, [mode_info_block.bpp]
	jne .next_mode

	; If we make it here, we've found the correct mode!
	mov ax, [.width]
	mov word[vbe_screen.width], ax
	mov ax, [.height]
	mov word[vbe_screen.height], ax
	mov eax, [mode_info_block.framebuffer]
	mov dword[vbe_screen.physical_buffer], eax
	mov ax, [mode_info_block.pitch]
	mov word[vbe_screen.bytes_per_line], ax
	mov eax, 0
	mov al, [.bpp]
	mov byte[vbe_screen.bpp], al
	shr eax, 3
	mov dword[vbe_screen.bytes_per_pixel], eax

	; Set the mode
	push es
	mov ax, 0x4F02
	mov bx, [.mode]
	or bx, 0x4000			; enable LFB
	mov di, 0			; not sure if some BIOSes need this... anyway it doesn't hurt
	int 0x10
	pop es

	cmp ax, 0x4F
	jne .error

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

.width				dw 0
.height				dw 0
.bpp				db 0
.segment			dw 0
.offset				dw 0
.mode				dw 0


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

g_GDT_end:

g_GDTDesc:  dw g_GDTDesc - g_GDT - 1    ; limit = size of GDT
            dd g_GDT                    ; address of GDT

g_BootDrive: db 0

section .data

vbe_error_msg: db "VBE Error: Failed to set graphics mode.", 0

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

vbe_screen:
    .width           resw 1
    .height          resw 1
    .bpp             resb 1
                     resb 3  ; Padding to align bytes_per_pixel to a 4-byte boundary
    .bytes_per_pixel resd 1
    .bytes_per_line  resw 1
                     resb 6  ; Padding to align physical_buffer to offset 20
    .physical_buffer resd 1