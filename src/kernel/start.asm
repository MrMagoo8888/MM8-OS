bits 32
section .text
KERNEL_LOAD_ADDR equ 0x100000 ; Address where the kernel is loaded in memory
KERNEL_STACK_TOP equ KERNEL_LOAD_ADDR - 0xF ; Stack grows down from just below the kernel

%include "bootinfo.inc"

; Linker needs this to resolve the C function call
extern start
extern __bss_start
extern __end
extern draw_pixel

global _start

_start:
    ; The bootloader passed the boot drive in edx. Save it immediately.
    ; Use a non-volatile register like ebx to avoid conflicts with ebp (base pointer).
    mov ebx, edx
    ; Set up the stack first. We'll place it right before the kernel's code.
    
    mov esp, KERNEL_STACK_TOP
    ;move esp, 0x9FFFC  ; Example stack top address

    ; Clear the BSS section (uninitialized global variables) (Does not mess up vbe i dont think)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    rep stosb

    ; Draw a pixel to confirm we are in the kernel's assembly entry point
    mov eax, 100          ; x coordinate
    mov edx, 150          ; y coordinate
    mov ecx, 0x00FF00FF   ; color (magenta)
    call draw_pixel
    add esp, 12         ; Clean up the 3 arguments (3 * 4 bytes) from the stack

    ; The bootloader passed the boot drive number in dl.
    ; Push it as an argument for the C start() function.
    push ebx
    call start

    cli
.hang:
    hlt
    jmp .hang

; void draw_pixel(int x, int y, uint32_t color);
draw_pixel:
    [bits 32]
    push ebp
    mov ebp, esp
    pushad ; Save all general-purpose registers

    ; Get arguments from stack
    mov ecx, [ebp + 8]      ; x
    mov edx, [ebp + 12]     ; y
    mov eax, [ebp + 16]     ; color
    
    mov esi, BOOTINFO_ADDR

    mov edi, [esi + bootinfo.vbe_physical_buffer]   ; Framebuffer address
    mov ebx, [esi + bootinfo.vbe_pitch]             ; Bytes per scanline (pitch)
    imul edx, ebx                           ; y * pitch
    add edi, edx                            ; edi = framebuffer + y * pitch

    imul ecx, 4                             ; x * 4 (bytes per pixel)
    add edi, ecx                            ; edi = framebuffer + y * pitch + x * 4
    mov [edi], eax                          ; Write color to pixel address

    popad
    mov esp, ebp
    pop ebp
    ret 12 ; Clean up 3 arguments * 4 bytes/arg from stack

