bits 16

section _ENTRY class=Code

extern _cstart_
global entry

entry:
    cli
    mov ax, ds
    mov ss, ax
    mov sp, 0
    mov bp, sp
    sti

    ; expect boot drive in dl, send as argument to cstart func
    xor dh, dh
    push dx
    call _cstart_

    cli
    hlt