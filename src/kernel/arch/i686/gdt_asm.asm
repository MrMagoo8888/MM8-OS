[bits 32]

global i686_GDT_Load
global i686_EnterUserMode

; void i686_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);
i686_GDT_Load:
    push ebp
    mov ebp, esp

    ; Load the GDT pointer
    mov eax, [ebp + 8]
    lgdt [eax]

    ; Reload Code Segment (CS) using a far return
    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf

.reload_cs:
    ; Reload Data Segments
    mov ax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

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

    ; Prepare the stack for IRET
    ; The CPU pops: EIP, CS, EFLAGS, ESP, SS
    push 0x23               ; SS (User Data Segment)
    push ebx                ; ESP (User Stack Pointer)
    
    pushf                   ; Push current EFLAGS
    pop eax
    or eax, 0x200           ; Set IF (Interrupt Flag) so user mode can receive interrupts
    push eax
    
    push 0x1B               ; CS (User Code Segment: Index 3: 0x18 | RPL 3 = 0x1B)
    push edx                ; EIP (Entry Point)

    iret                    ; The magic jump to Ring 3