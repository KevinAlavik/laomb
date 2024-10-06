[bits 32]
section .text
extern idt_default_handler


%macro ISR_NOERRORCODE 1
global idt_%1:
idt_%1:
    push 0
    push %1
    jmp isr_common
%endmacro

%macro ISR_ERRORCODE 1
global idt_%1:
idt_%1:
    push %1
    jmp isr_common
%endmacro

isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov eax, cr2
    push eax
    mov eax, cr3
    push eax

    mov eax, dr0
    push eax
    mov eax, dr1
    push eax
    mov eax, dr2
    push eax
    mov eax, dr3
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push eax    

    mov eax, idt_default_handler
    call eax

    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

ISR_NOERRORCODE 0
ISR_NOERRORCODE 1
ISR_NOERRORCODE 2
ISR_NOERRORCODE 3
ISR_NOERRORCODE 4
ISR_NOERRORCODE 5
ISR_NOERRORCODE 6
ISR_NOERRORCODE 7
ISR_ERRORCODE 8
ISR_NOERRORCODE 9
ISR_ERRORCODE 10
ISR_ERRORCODE 11
ISR_ERRORCODE 12
ISR_ERRORCODE 13
ISR_ERRORCODE 14
ISR_NOERRORCODE 15
ISR_NOERRORCODE 16
ISR_ERRORCODE 17
ISR_NOERRORCODE 18
ISR_NOERRORCODE 19
ISR_NOERRORCODE 20
ISR_ERRORCODE 21
ISR_NOERRORCODE 22
ISR_NOERRORCODE 23
ISR_NOERRORCODE 24
ISR_NOERRORCODE 25
ISR_NOERRORCODE 26
ISR_NOERRORCODE 27
ISR_NOERRORCODE 28
ISR_NOERRORCODE 29
ISR_NOERRORCODE 30
ISR_NOERRORCODE 31

%assign i 32
%rep 224
ISR_NOERRORCODE i
%assign i i+1
%endrep

section .data
global isr_table:
isr_table:
%assign i 0
%rep 256
	dd idt_%+i
%assign i i+1
%endrep