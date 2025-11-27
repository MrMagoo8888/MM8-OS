bits 32
section .text

extern kernel_main
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
    

    ; --- KERNEL ENTRY DEBUG ---
    ; Draw a red pixel at (0, 1) to confirm we've reached the kernel's _start.
    ; The white pixel at (0, 0) is from the bootloader.
    mov edi, [ebp + 8]      ; Get vbe_screen struct pointer from the stack
    mov edi, [edi + 20]     ; Get physical_buffer from vbe_screen (correct offset is 20)
    add edi, 4              ; Move to the next pixel (x=1, y=0) to not overwrite stage2's pixel
    mov dword [edi], 0x0000FF00 ; Write a green pixel

    ; Push arguments for the C start() function in reverse order.
    push dword [ebp + 8] ; Push vbe_info pointer
    push dword [ebp + 4] ; Push bootDrive
    ; call kernel_main

    cli
.hang:
    hlt
    jmp .hang
