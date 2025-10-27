bits 32
section .text

; Linker needs this to resolve the C function call
extern start
extern __bss_start
extern __end

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

    ; The boot drive number is in dl, push it as an argument for start.
    push edx
    call start

    cli
.hang:
    hlt
    jmp .hang
