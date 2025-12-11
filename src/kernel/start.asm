bits 32
section .text
%include "bootinfo.inc"

KERNEL_LOAD_ADDR equ 0x100000
KERNEL_STACK_TOP equ KERNEL_LOAD_ADDR - 0xFFFF ; Stack grows down from just below the kernel

; Linker needs this to resolve the C function call
extern start
extern __bss_start
extern __end

global _start

_start:
    ; The bootloader passed the boot drive in edx. Save it immediately.
    mov ebp, edx

    ; Set up the stack first. We'll place it right before the kernel's code.
    
    mov esp, KERNEL_STACK_TOP
    ;move esp, 0x9FFFC  ; Example stack top address

    ; Clear the BSS section (uninitialized global variables)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    rep stosb

    
    ; Draw some pixels to test the framebuffer
    ; draw_pixel(x, y, color)

    ; Stage2 is White magenta yellow
    mov ecx, 350           ; x
    mov edx, 200          ; y
    mov eax, 0x00FF0000     ; Red
    call draw_pixel

    mov ecx, 450          ; x
    mov edx, 200           ; y
    mov eax, 0x0000FF00     ; Green
    call draw_pixel

    mov ecx, 550            ; x
    mov edx, 200            ; y
    mov eax, 0x000000FF     ; Blue
    call draw_pixel

    ; The bootloader passed the boot drive number in dl.
    ; Push it as an argument for the C start() function.
    push ebp
    call start

    cli
.hang:
    hlt
    jmp .hang

draw_pixel:
    [bits 32]
    pushad

    mov esi, BOOTINFO_ADDR

    mov edi, [esi + bootinfo.vbe_physical_buffer]   ; Framebuffer address
    mov ebx, [esi + bootinfo.vbe_pitch]             ; Bytes per scanline (pitch)
    imul edx, ebx                           ; y * pitch
    add edi, edx                            ; edi = framebuffer + y * pitch
    
    imul ecx, 4                             ; x * 4 (bytes per pixel)
    add edi, ecx                            ; edi = framebuffer + y * pitch + x * 4

    mov [edi], eax                          ; Write color to pixel address
    popad
    ret
