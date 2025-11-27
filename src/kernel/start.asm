bits 32
section .text

extern kernel_main
extern __bss_start
extern __end

global _start

_start:
    ; Function prologue: Set up the stack frame
    push ebp
    mov ebp, esp

    ; Clear the BSS section (uninitialized global variables)
    ; mov edi, __bss_start
    ; mov ecx, __end
    ; sub ecx, edi
    ; mov al, 0
    ; rep stosb

    ; The arguments from the bootloader are on the stack.
    ; After our prologue: [ebp+12] = framebuffer_address, [ebp+8] = bootDrive
    

    ; --- KERNEL ENTRY DEBUG ---
    ; Draw a red pixel at (0, 1) to confirm we've reached the kernel's _start.
    ; The white pixel at (0, 0) is from the bootloader.
    mov edi, [ebp + 12]     ; Get framebuffer address directly from the stack
    add edi, 64              ; Move to the next pixel (x=1, y=0) to not overwrite stage2's pixel
    mov dword [edi], 0x00FF0000 ; Write a green pixel

    ; Push arguments for the C start() function in reverse order.
    push dword [ebp + 12] ; Push framebuffer_address
    push dword [ebp + 8]  ; Push bootDrive
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
