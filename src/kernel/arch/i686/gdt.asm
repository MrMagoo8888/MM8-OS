[bits 32]

; void __attribute__((cdecl)) i686_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);
global i686_GDT_Load
global i686_EnterUserMode

i686_GDT_Load:
    
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame
    
    ; load gdt
    mov eax, [ebp + 8]
    lgdt [eax]

    ; reload code segment
    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf

.reload_cs:

    ; reload data segments
    mov ax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; void i686_EnterUserMode(void* entryPoint, void* userStack);
i686_EnterUserMode:
    mov edx, [esp + 4]      ; Target EIP (Entry Point)
    mov ebx, [esp + 8]      ; Target ESP (User Stack)

    ; Set segment registers to User Data selector (Index 4: 0x20 | RPL 3 = 0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Prepare the stack for IRET (Interrupt Return)
    ; The CPU pops: EIP, CS, EFLAGS, ESP, SS
    push 0x23               ; SS (User Data Segment)
    push ebx                ; ESP (User Stack Pointer)
    
    pushf                   ; Push current EFLAGS
    pop eax
    or eax, 0x200           ; Set IF (Interrupt Flag) so user mode can receive interrupts
    push eax
    
    push 0x1B               ; CS (User Code Segment: Index 3: 0x18 | RPL 3 = 0x1B)
    push edx                ; EIP (Entry Point)

    iret                    ; Perform the switch to Ring 3