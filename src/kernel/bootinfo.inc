; This file contains structures and constants shared between the bootloader and the kernel.

BOOTINFO_ADDR equ 0x9000 ; A safe, well-known memory address below 1MB.

struc bootinfo
    .vbe_physical_buffer: resd 1
    .vbe_pitch:           resd 1
    .vbe_width:           resw 1
    .vbe_height:          resw 1
    .vbe_bpp:             resb 1
endstruc

; This makes it easier to access members, e.g., [eax + bootinfo.vbe_pitch]