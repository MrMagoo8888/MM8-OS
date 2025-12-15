bits 32
section .text
%include "bootinfo.inc"

KERNEL_LOAD_ADDR equ 0x100000 ; Address where the kernel is loaded in memory
KERNEL_STACK_TOP equ KERNEL_LOAD_ADDR - 0xF ; Stack grows down from just below the kernel

; Linker needs this to resolve the C function call and BSS symbols
extern main
extern __bss_start
extern __end

global _start

_start:
    ; The bootloader passed the boot drive in edx. Save it immediately.
    ; We will also pass the bootinfo struct address, which the bootloader
    ; should place in a known register (e.g., esi) before jumping here.

    ; Set up the stack first. We'll place it right before the kernel's code.
    mov esp, KERNEL_STACK_TOP

    ; Clear the BSS section (uninitialized global variables)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    rep stosb

    ; The bootloader should pass boot information (like the bootinfo address)
    ; in registers. For now, we call main without arguments.
    call main

    cli
.hang:
    hlt
    jmp .hang