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

    ; The arguments from the bootloader are on the stack.
    ; [esp+8] = vbe_info pointer, [esp+4] = bootDrive
    ; We must retrieve them before changing the stack pointer.
    mov ebp, esp

    ; Set up the stack. We'll place it right before the kernel's code at 0x100000
    ;mov esp, 0x9FFFC ; Stack top just below 0xA0000, 4-byte aligned
    mov esp, 0x10000

    ; Push arguments for the C start() function in reverse order.
    push dword [ebp + 8] ; Push vbe_info pointer
    push dword [ebp + 4] ; Push bootDrive
    call start

    cli
.hang:
    hlt
    jmp .hang
