bits 32
section .text

; Linker needs this to resolve the C function call
extern start
extern __bss_start
extern __end
extern vbe_screen

global _start

_start:
    ; Clear the BSS section (uninitialized global variables)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    rep stosb

    ; Set up the stack. We'll place it right before the kernel's code at 0x100000
    mov esp, 0x9FFFF

    ; The bootloader passes arguments to us in registers:
    ; EAX = pointer to VBE screen info
    ; EBX = boot drive number
    push ebx      ; Push boot drive as the second argument
    push eax      ; Push vbe_screen pointer as the first argument
    call start

    cli
.hang:
    hlt
    jmp .hang
