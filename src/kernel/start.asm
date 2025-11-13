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

    ; The bootloader already set up a safe stack for us at 0x90000.
    ; We should not change esp here.

    ; --- KERNEL ENTRY DEBUG ---
    ; Draw a red pixel at (0, 1) to confirm we've reached the kernel's _start.
    ; The white pixel at (0, 0) is from the bootloader.
    mov edi, [ebp + 8]      ; Get vbe_info pointer from the stack
    mov edi, [edi + 12]     ; Get physical_buffer from vbe_info (offset of physical_buffer is 12)
    add edi, 4              ; Move to the next pixel (x=1, y=0)
    mov dword [edi], 0x00FF0000 ; Write a red pixel

    ; Push arguments for the C start() function in reverse order.
    push dword [ebp + 8] ; Push vbe_info pointer
    push dword [ebp + 4] ; Push bootDrive
    call start

    cli
.hang:
    hlt
    jmp .hang
