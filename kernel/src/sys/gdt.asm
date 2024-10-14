[bits 32]
section .data

g_tss:
    times 26 dd 0

g_gdt:
    ; Null descriptor
    dw 0x0000                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0x00                  ; access
    db 0x00                  ; granularity
    db 0x00                  ; base_high

    ; Code segment descriptor
    dw 0xFFFF                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0x9A                  ; access
    db 0xC0                  ; granularity
    db 0x00                  ; base_high

    ; Data segment descriptor
    dw 0xFFFF                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0x92                  ; access
    db 0xC0                  ; granularity
    db 0x00                  ; base_high

    ; User code segment descriptor
    dw 0xFFFF                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0xFA                  ; access
    db 0xC0                  ; granularity
    db 0x00                  ; base_high

    ; User data segment descriptor
    dw 0xFFFF                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0xF2                  ; access
    db 0xC0                  ; granularity
    db 0x00                  ; base_high

    ; TSS descriptor
    dw 0x0067                ; limit
    dw 0x0000                ; base_low
    db 0x00                  ; base_mid
    db 0x89                  ; access
    db 0x00                  ; granularity
    db 0x00                  ; base_high
g_gdt_end:

g_gdt_ptr:
    dw g_gdt_end - g_gdt - 1 ; limit (size of g_gdt - 1)
    dd g_gdt                 ; base (address of g_gdt)

section .text

global gdt_load
gdt_load:
    push ebp
    mov ebp, esp

    lgdt [g_gdt_ptr]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    push dword 0x08
    push dword @@1
    retf
@@1:
    pop ebp
    ret
    
global tss_init
tss_init:
    push ebp
    mov ebp, esp

    ;; TODO
    
    pop ebp
    ret