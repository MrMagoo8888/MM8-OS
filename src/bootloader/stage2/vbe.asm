; Minimal VBE set-mode + store FramebufferInfo at physical 0x9000
; NASM, 16-bit real mode. Call set_vbe_mode before switching to protected mode.

BITS 16
SECTION .text
global set_vbe_mode        ; <-- Export the symbol so entry.obj can link to it

MODEINFO_BUF  equ 0x8000
FBINFO_ADDR   equ 0x9000

; Choose a mode known to commonly exist in QEMU/BIOS.
; 0x118 = 1024x768x32bpp in many VBE implementations (adjust if needed).
%define VBE_MODE 0x118
%define VBE_LINEAR_FLAG 0x4000

; FramebufferInfo layout at FBINFO_ADDR (little-endian):
; dword framebuffer_phys
; dword width
; dword height
; dword pitch (bytes per scanline)
; byte  bpp

set_vbe_mode:
    pushf
    cli

    ; Request ModeInfo into MODEINFO_BUF: AX=0x4F01, ES:DI -> buf
    xor ax, ax
    mov ax, 0x4F01
    xor dx, dx
    mov di, MODEINFO_BUF
    mov es, dx
    int 0x10
    ; ignore failure for query (we'll check later)

    ; Set mode with linear framebuffer flag: AX=0x4F02, BX=mode|0x4000
    mov ax, 0x4F02
    mov bx, VBE_MODE | VBE_LINEAR_FLAG
    int 0x10
    jc .set_fail

    ; Now MODEINFO_BUF contains ModeInfo structure
    ; PhysBasePtr (dword) at offset 0x40 (64)
    ; XResolution (word) at offset 0x12 (18)
    ; YResolution (word) at offset 0x14 (20)
    ; BytesPerScanLine (word) at offset 0x16 (22)
    ; BitsPerPixel (byte) at offset 0x19 (25)

    ; Copy PhysBasePtr (dword)
    mov si, MODEINFO_BUF + 64
    mov di, FBINFO_ADDR
    mov ax, [si]
    mov [di], ax
    mov ax, [si+2]
    mov [di+2], ax

    ; Copy XResolution (word) -> store as dword at +4
    mov ax, [MODEINFO_BUF + 18]
    mov [di+4], ax
    mov word [di+6], 0

    ; Copy YResolution (word) -> store as dword at +8
    mov ax, [MODEINFO_BUF + 20]
    mov [di+8], ax
    mov word [di+10], 0

    ; BytesPerScanLine (word) -> store as dword at +12
    mov ax, [MODEINFO_BUF + 22]
    mov [di+12], ax
    mov word [di+14], 0

    ; BitsPerPixel (byte) -> store at +16
    mov al, [MODEINFO_BUF + 25]
    mov [di+16], al

    ; success
    sti
    popf
    ret

.set_fail:
    sti
    popf
    ret